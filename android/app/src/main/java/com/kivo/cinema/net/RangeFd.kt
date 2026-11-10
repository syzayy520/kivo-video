package com.kivo.cinema.net

import android.os.Handler
import android.os.HandlerThread
import android.os.ParcelFileDescriptor
import android.os.ProxyFileDescriptorCallback
import android.os.storage.StorageManager
import android.system.ErrnoException
import android.system.OsConstants
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 把任意 [ByteSource] 暴露成一个可随机读的文件描述符（243 D3）。
 *
 * 这条路径就是 SMB / WebDAV / 本地播放的**唯一**传输层：
 * `ByteSource → ParcelFileDescriptor → FFmpeg（软解）/ MediaExtractor（硬解）`。
 * 243 之前 SMB 还要多走一段"Kotlin 本地 HTTP 代理 + HTTP 客户端"的环回，
 * 每次读 32 KiB、一把全局锁，4K 片源每秒几百次往返；这里去掉那两层。
 *
 * 语义要点（与系统 FUSE 回调契约对齐）：
 * - `onRead` 必须填满请求区间，只有真到文件尾才返回短读；系统会把短读当 EOF；
 * - 每次 [openDescriptor] 有独立的回调线程，互不阻塞；
 * - [cancel] 只置标志，不 join——调用方（原生侧）会先把阻塞读打断再释放。
 */
class RangeFd(
    private val source: ByteSource,
    private val storage: StorageManager,
    /**
     * 单次 `onRead` 允许为"等网络回来"阻塞多久（毫秒）。
     *
     * 网络抖动、Wi-Fi 重连、服务器从休眠唤醒都只需要几秒；这段时间里解码器
     * 停在最后一帧等数据，用户看到的是"卡一下"，不是"播放失败"。
     * 超过这个上限才判定为真故障 —— 这也是唯一会写 [failureReason] 的路径。
     */
    private val readPatienceMs: Long = DEFAULT_PATIENCE_MS,
    /**
     * 链路可达性探针（250 D3）。非 null 时起一个 1 秒轮询：**断网立刻置 [stalled]**，
     * 不等 `onRead` 在 JNI 里超时。
     *
     * 为什么必须另起一条检测而不靠读超时：真机实测（K40，4K 硬解），
     * 即便把 libsmb2 的 `smb2_set_timeout` 设成 3 s，断网后 `smb2_pread` 仍然
     * 15 s 才返回 —— `stalled` 因此在断网 15 s 后才为真，界面一直显示"正在播放"。
     * 而"有没有网"是系统事实，本来就不该等 IO 报错才知道。
     */
    private val online: (() -> Boolean)? = null,
    /**
     * 是否拥有 [source] 的生命周期。默认 true。
     *
     * false 用于"临时 fd"（252 路由探测）：探测结束要释放这个 fd，但**不能**把
     * 正在用的数据源一起关掉。关闭代理 fd 会让阻塞中的读永远等不到回调线程，
     * 所以探测的中断手段是 [cancel]（回调下一次读立刻抛 EIO），不是 close。
     */
    private val ownsSource: Boolean = true,
) : RangeSource {

    private val cancelled = AtomicBoolean(false)
    private val readers = CopyOnWriteArrayList<Reader>()

    /**
     * 链路看门狗（250 D3）。只在传了 [online] 时启动；1 秒一次，掉线立刻点亮 [stalled]，
     * 让界面在"读还没失败"的时候就知道该提示"网络已断开"。
     */
    private val linkWatchdog: Thread? = online?.let { probe ->
        Thread({
            while (!cancelled.get()) {
                try {
                    Thread.sleep(LINK_POLL_MS)
                } catch (_: InterruptedException) {
                    return@Thread
                }
                if (cancelled.get()) return@Thread
                val reachable = runCatching { probe() }.getOrDefault(true)
                if (!reachable && !stalled) {
                    stalled = true
                    android.util.Log.i("KivoRangeFd", "link lost (watchdog)")
                }
            }
        }, "KivoSmbLink").apply {
            isDaemon = true
            start()
        }
    }

    override val size: Long = source.size()

    @Volatile
    override var failureReason: String = ""
        private set

    /** 正在重试窗口内（界面据此显示"正在重连…"）。 */
    @Volatile
    override var stalled: Boolean = false
        private set

    /** 字节源在构造时已就绪（大小已知），没有额外准备动作。 */
    override fun open() = Unit

    override fun openDescriptor(): ParcelFileDescriptor {
        check(size > 0 && !cancelled.get()) { "片源未就绪或已取消" }
        val reader = Reader()
        readers += reader
        try {
            return storage.openProxyFileDescriptor(
                ParcelFileDescriptor.MODE_READ_ONLY, reader, Handler(reader.thread.looper),
            ) ?: error("系统无法创建文件描述符")
        } catch (e: Exception) {
            reader.onRelease()
            throw e
        }
    }

    override fun cancel() {
        cancelled.set(true)
        stalled = false
    }

    override fun close() {
        cancel()
        readers.toList().forEach { it.onRelease() }
        if (ownsSource) runCatching { source.close() }
    }

    private inner class Reader : ProxyFileDescriptorCallback() {
        val thread = HandlerThread("KivoRangeFd").apply { start() }
        private val released = AtomicBoolean(false)

        override fun onGetSize(): Long = size

        override fun onRead(offset: Long, requested: Int, data: ByteArray): Int {
            if (cancelled.get() || released.get()) throw ErrnoException("read", OsConstants.EIO)
            if (offset < 0 || requested <= 0) return 0
            if (offset >= size) return 0
            val want = minOf(requested.toLong(), data.size.toLong(), size - offset).toInt()
            var filled = 0
            var waited = 0L
            var attempt = 0
            var stalledBy: Exception? = null
            while (filled < want) {
                try {
                    val read = source.readAt(offset + filled, data, filled, want - filled)
                    if (read < 0) break          // ByteSource 契约：-1 = 文件尾
                    if (read == 0) break
                    filled += read
                    if (stalled || stalledBy != null) {
                        android.util.Log.i(
                            "KivoRangeFd",
                            "read recovered offset=${offset + filled} after ${waited}ms",
                        )
                        stalledBy = null
                        waited = 0L
                        attempt = 0
                        stalled = false
                    }
                } catch (e: Exception) {
                    // 取消优先：用户退出/切源时必须立刻放手，不能让重试拖住 teardown。
                    if (cancelled.get() || released.get()) throw ErrnoException("read", OsConstants.EIO)
                    stalledBy = e
                    if (waited >= readPatienceMs) {
                        // 失败原因必须同时进日志与 UI：只留一句"读取中断"无法归因。
                        android.util.Log.e(
                            "KivoRangeFd",
                            "read failed offset=$offset want=$want after ${waited}ms: ${e.message}", e,
                        )
                        stalled = false
                        failureReason = e.message?.takeIf { it.isNotBlank() }
                            ?: "读取失败（${e.javaClass.simpleName}）"
                        throw ErrnoException("read", OsConstants.EIO)
                    }
                    // 第一次失败就点亮 stalled：界面从这一刻开始说"正在重连"，
                    // 而不是等 20 秒后才报错。
                    if (!stalled) {
                        stalled = true
                        android.util.Log.i("KivoRangeFd", "read stalled offset=$offset: ${e.message}")
                    }
                    // 退避 250 → 500 → 1000 ms，之后维持 1 s：既不打爆服务器，
                    // 也能在网络恢复后 1 秒内接上。
                    val pause = minOf(250L shl minOf(attempt, 2), 1000L)
                    attempt++
                    try {
                        Thread.sleep(pause)
                    } catch (_: InterruptedException) {
                        Thread.currentThread().interrupt()
                        throw ErrnoException("read", OsConstants.EIO)
                    }
                    waited += pause
                }
            }
            return filled
        }

        override fun onRelease() {
            if (!released.compareAndSet(false, true)) return
            readers.remove(this)
            thread.quitSafely()
        }
    }

    companion object {
        /** 20 秒：覆盖 Wi-Fi 重连（通常 2–5 秒）与路由器/NAS 短时重启。 */
        private const val DEFAULT_PATIENCE_MS = 20_000L

        /** 链路轮询间隔。1 秒 = 用户能忍受的"卡了一下才发现"，又不至于耗电。 */
        private const val LINK_POLL_MS = 1_000L
    }
}
