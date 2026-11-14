package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.NetDiag
import org.json.JSONObject
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit

/**
 * 273 v19：**demuxer 缓存归因诊断** —— 定期读 mpv 的缓存状态并落盘。
 *
 * ## 为什么需要它
 *
 * 播放期实测 `TOTAL PSS` = 584.6 MiB，超出 < 500 MB 的目标（273-D 的判据）。
 * Native Heap 占 336 MB，但**谁占的没有证据**：可能是 mpv 的 demux 缓存
 * （前向 `demuxer-max-bytes` 64 MiB + 回看 16 MiB）、我们的取数环（每读者 8 MiB）、
 * MediaCodec 的解码缓冲、或者预览内核那**第二套** mpv。
 *
 * **没有归因就没有优化** —— 拍脑袋调参数是在赌，赌输了还欠了一笔技术债。
 *
 * ## 读的是什么
 *
 * mpv 的 `demuxer-cache-state`（节点属性，mpv 0.39）：
 * · `fw-bytes`      前向重读队列的字节数（**还会**往源站要多少）
 * · `total-bytes`   整个包队列的字节数（demux 缓存实际占用）
 * · `cache-duration` 缓存覆盖的媒体时长
 * · `underrun`       取数线程满足不了解码器（**demux 侧**欠载，与 AO 欠载是两回事）
 * · `raw-input-rate` 估算的输入速率
 *
 * ## 为什么是永久件
 *
 * 内存与"超买"两个缺口都要靠它**反复复算**；一次性探针收口时会被删掉，
 * 证据就断了 —— 而断掉的证据等于没测过。
 */
object MpvCacheDiag {

    /** 连续读不到多少次就摘除（会话已释放时属性读不到）。 */
    private const val MAX_MISSES = 3

    private const val PERIOD_MS = 2_000L

    @Volatile
    private var session: MpvSession? = null

    private var worker: ScheduledExecutorService? = null

    private var misses = 0

    /** 是否已经读到过一次（读不到 ≠ 会话没了 —— 媒体还没加载时属性根本不存在）。 */
    private var seenOnce = false

    /**
     * 挂上会话，开始采样（幂等；换会话会重新开始计）。
     *
     * 只给**主播放**内核挂 —— 预览内核不挂，否则两套缓存的数字混在一起。
     */
    @Synchronized
    fun attach(target: MpvSession) {
        if (session === target && worker != null) return
        stop()
        session = target
        misses = 0
        seenOnce = false
        val executor = Executors.newSingleThreadScheduledExecutor { r ->
            Thread(r, "KivoCacheDiag").apply { isDaemon = true }
        }
        worker = executor
        executor.scheduleWithFixedDelay({ sample() }, 0, PERIOD_MS, TimeUnit.MILLISECONDS)
    }

    /** 摘掉会话并停采样（幂等）。 */
    @Synchronized
    fun stop() {
        worker?.shutdownNow()
        worker = null
        session = null
    }

    private fun sample() {
        val target = session ?: return stop()
        val raw = target.property("demuxer-cache-state")
        if (raw == null) {
            // 媒体还没加载时属性不存在 —— 那不是"会话没了"，继续等。
            // 只有**读到过**之后再连续读不到，才说明会话被释放了。
            if (!seenOnce) return
            if (++misses >= MAX_MISSES) stop()
            return
        }
        seenOnce = true
        misses = 0
        val node = runCatching { JSONObject(raw) }.getOrNull() ?: return
        NetDiag.log(
            "[cache] fw=${mib(node.optLong("fw-bytes"))} " +
                "total=${mib(node.optLong("total-bytes"))} " +
                "dur=${"%.1f".format(node.optDouble("cache-duration"))}s " +
                "underrun=${node.optBoolean("underrun")} " +
                "rate=${mib(node.optLong("raw-input-rate"))}/s"
        )
    }

    private fun mib(bytes: Long): String = "%.1fM".format(bytes / 1048576.0)
}
