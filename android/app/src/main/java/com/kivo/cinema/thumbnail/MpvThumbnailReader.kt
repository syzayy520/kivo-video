package com.kivo.cinema.thumbnail

import android.content.Context
import android.graphics.Bitmap
import android.os.Handler
import android.os.HandlerThread
import android.os.SystemClock
import android.util.LruCache
import com.kivo.cinema.mpv.LocalRangeServer
import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.ByteSource
import java.io.File
import java.nio.ByteBuffer

/**
 * 网络/无索引源的预览帧读取器（v3.1b）：**mpv 内核第二实例**。
 *
 * 【为什么不是 MediaExtractor】真机定论（2026-11-12，shelter.mkv 12GB 无索引）：
 * MediaExtractor 走 MKV Cues，缺 Cues 时 setDataSource 线性扫全片，单解码线程
 * 被首请求卡死（T+60 dump 仍 handler busy）⇒ 整段拖动空框。mpv 的 matroska
 * demux 无索引按字节插值定位（主播放 seek 真机 28–49ms 已证）⇒ 预览帧走内核。
 *
 * 【形态】vo=libmpv + 离屏 FBO 取帧（attachCapture 自起渲染线程）、
 * ao=null 不出声、pause=yes；seek 取**关键帧**（与主内核 seek 同口径：
 * 真机实测 exact 解码每帧 1.6–2.2s，超出拖动手感；关键帧 ≤1 GOP 偏差是
 * 预览浮窗的行业口径）→ 最新 RGBA 帧 → Bitmap。
 * 代理形态与主播放共用 [LocalRangeServer]，但实例独立 ⇒ 主链路零干扰。
 *
 * 【为什么 FBO+glReadPixels 而不是 ImageReader】真机红清屏二分定论：
 * Adreno 650 上 EGL window surface→ImageReader 的缓冲内容传输坏（swap 成功、
 * timestamp 推进但永远 gralloc 填充色，连自绘 glClear 都到不了）；去
 * EGL_RECORDABLE_ANDROID 亦无效。纹理 FBO 读回不走那条坏传输（真机已证可靠）。
 * 取帧走 worker 轮询 readCaptureFrame（native 按序号去重，只给新帧）。
 *
 * 【预热与常驻】挂载即 [prewarm]（延后 1.2s 让主内核先起播）；内核随会话常驻
 * （重初始化真机 ~8s，闲释放会让每次隔段拖动重新冷启），demux 缓存压到 8MiB
 * 控内存。latest-wins/秒级 LRU/取证计数：与 [ThumbnailReader] 同构。
 */
class MpvThumbnailReader(
    private val appContext: Context,
    private val input: ByteSource,
    private val cacheDir: File,
    private val cacheSize: Int = DEFAULT_CACHE_SIZE,
) : PreviewFrameReader {

    private val cache = object : LruCache<Int, Bitmap>(cacheSize) {
        override fun entryRemoved(evicted: Boolean, key: Int?, oldValue: Bitmap?, newValue: Bitmap?) {
            if (evicted) evictions += 1
        }
    }

    private val thread = HandlerThread("mpv-thumb").apply { start() }
    private val handler = Handler(thread.looper)

    override var onThumbnailReady: ((positionUs: Long, bitmap: Bitmap) -> Unit)? = null
    override var onUnavailable: ((reason: String) -> Unit)? = null

    private var server: LocalRangeServer? = null
    private var registeredId: String? = null
    private var session: MpvSession? = null
    private var initialized = false
    private var initFailed = false
    private var closing = false
    private var unavailableReason = "预览帧不可用"

    @Volatile private var latestUs = -1L
    @Volatile private var latestRequestAtMs = 0L

    // ---- 取证计数（handler 线程读写）----
    private var requests = 0
    private var cacheHits = 0
    private var decodeFailures = 0
    private var evictions = 0
    private var initMs = -1L
    private var coldFirstLatencyMs = -1L
    private val warmLatenciesMs = ArrayList<Long>()
    // 取证：实际取帧数（与渲染层 diag 的 cap # 对照）。
    private var framesAcquired = 0
    private val grabLog = ArrayList<String>()

    override fun seekTo(positionUs: Long) {
        if (closing) return
        latestRequestAtMs = SystemClock.elapsedRealtime()
        latestUs = positionUs
        handler.removeCallbacks(decodeWorker)
        handler.post(decodeWorker)
    }

    override fun prewarm() {
        if (closing) return
        // 让主内核先完成起播读盘：两条 SMB 读路径同时抢链路会拖慢首帧。
        handler.postDelayed({ if (!closing) ensureInitialized() }, PREWARM_DELAY_MS)
    }

    private val decodeWorker = Runnable {
        if (closing) return@Runnable
        val positionUs = latestUs
        val requestAt = latestRequestAtMs
        requests += 1
        val key = (positionUs / 1_000_000).toInt()
        cache.get(key)?.let { cached ->
            cacheHits += 1
            recordLatency(requestAt)
            onThumbnailReady?.invoke(positionUs, cached)
            return@Runnable
        }
        ensureInitialized()
        val bitmap = if (initialized) grabFrameAt(positionUs) else null
        if (bitmap != null) {
            cache.put(key, bitmap)
            recordLatency(requestAt)
            onThumbnailReady?.invoke(positionUs, bitmap)
        } else {
            decodeFailures += 1
            onUnavailable?.invoke(unavailableReason)
        }
    }

    private fun recordLatency(requestAtMs: Long) {
        val latency = SystemClock.elapsedRealtime() - requestAtMs
        if (coldFirstLatencyMs < 0) coldFirstLatencyMs = latency
        else warmLatenciesMs.add(latency)
    }

    private fun ensureInitialized() {
        if (initialized || initFailed) return
        val start = SystemClock.elapsedRealtime()
        try {
            val proxy = LocalRangeServer.start()
            server = proxy
            val id = proxy.register(input)
            registeredId = id
            val native = MpvSession.create(appContext)
            session = native
            native.initialize(
                vo = "libmpv",
                ao = "",
                logFile = File(cacheDir, "mpv-thumb.log").absolutePath,
            )
            native.setProperty("pause", "yes")
            // 预览只要关键帧。native seek(exact=false) 发的是 `absolute`，而 mpv
            // hr-seek 默认=absolute ⇒ 仍精确落位、从关键帧前向解码到目标（真机日志
            // "hr-seek, skipping to 90" → 单帧 4.6s 主因）。no 关掉前向解码。
            native.setProperty("hr-seek", "no")
            // 预览不出声、不要字幕。真机日志 audio/1 堆到 8MiB 上限触发
            // "Too many packets" 阻塞视频出帧；aid/sid=no 把 demux 队列全留给视频。
            native.setProperty("aid", "no")
            native.setProperty("sid", "no")
            native.setProperty("demuxer-max-bytes", "8MiB") // 常驻内存闸：预览只要单帧。
            native.setProperty("demuxer-max-back-bytes", "4MiB")
            // pause⇒still 走 cache+blit 分支，该面 blit 不落（真机全透明）；dumb 模式直绘。
            native.setProperty("gpu-dumb-mode", "yes")
            native.attachCapture(WIDTH, HEIGHT)
            native.loadFile(proxy.urlFor(id))
            val deadline = System.currentTimeMillis() + 15_000
            while (System.currentTimeMillis() < deadline && native.durationSeconds() <= 0.0) {
                Thread.sleep(100)
            }
            check(native.durationSeconds() > 0.0) { "预览内核加载超时" }
            initMs = SystemClock.elapsedRealtime() - start
            initialized = true
            // 首帧 VO/解码器冷启动真机 ~1.7s：用 0 秒关键帧先暖掉，
            // 用户第一次拖动拿到的就是热路径延迟。
            grabFrameAt(0L)?.let { cache.put(0, it) }
        } catch (e: Exception) {
            initFailed = true
            unavailableReason = "预览内核起失败：${e.message}"
            releaseKernel()
        }
    }

    private fun grabFrameAt(positionUs: Long): Bitmap? {
        val native = session ?: return null
        val startMs = System.currentTimeMillis()
        var result = "ok"
        var bitmap: Bitmap? = null
        try {
            val target = positionUs / 1_000_000.0
            val prevPos = native.positionSeconds()
            val samePos = kotlin.math.abs(target - prevPos) < 0.5
            // 排空旧帧（同位置重取除外：当前帧就是目标，排空反而读不到）。
            if (!samePos) native.readCaptureFrame()
            native.seek(target, exact = false) // 关键帧口径：hr-seek=no 已禁前向解码。
            val frameDeadline = startMs + 6_000
            // 真黑场（片头黑底标题）整幅同色，hasContent 恒 false；2s 后接受同色帧兜底。
            val uniformFallbackMs = startMs + 2_000
            var bytes: ByteArray? = null
            var lastUniform: ByteArray? = null
            while (System.currentTimeMillis() < frameDeadline) {
                val candidate = native.readCaptureFrame()
                if (candidate != null) {
                    // 帧↔seek 对齐：连续快 seek 时 mpv 合并/滞后，仅序号去重会把上一次
                    // seek 的帧当本次结果（真机 off-by-one 级联、浮窗画面≠手指位置）。
                    // 只接受 time-pos 已离开旧位置的帧，斩断级联。
                    val pts = native.captureFramePts()
                    val isTarget = samePos || kotlin.math.abs(pts - prevPos) > 0.05
                    if (isTarget) {
                        if (hasContent(candidate)) {
                            bytes = candidate
                            break
                        }
                        lastUniform = candidate // 黑场帧只出现一次，留存供兜底而非再读。
                    }
                }
                if (lastUniform != null && System.currentTimeMillis() >= uniformFallbackMs) {
                    bytes = lastUniform
                    break
                }
                Thread.sleep(15)
            }
            if (bytes == null) {
                unavailableReason = "目标时刻出帧超时（片源读太慢）"
                result = "timeout"
            } else {
                framesAcquired += 1
                bitmap = bitmapFromRgba(bytes)
                if (bitmap != null && (framesAcquired == 1 || framesAcquired % 40 == 0)) runCatching { java.io.FileOutputStream(File(cacheDir, "thumb-dump-$framesAcquired.png")).use { bitmap.compress(Bitmap.CompressFormat.PNG, 100, it) } }
            }
        } catch (e: Exception) {
            unavailableReason = "预览帧读取失败：${e.message}"
            result = "exc:${e.javaClass.simpleName}"
        }
        if (grabLog.size >= 8) grabLog.removeAt(0)
        val landed = session?.captureFramePts() ?: -1.0
        grabLog.add("t=${positionUs / 1_000_000} land=${"%.1f".format(landed)} wait=${System.currentTimeMillis() - startMs}ms $result px0=${bitmap?.getPixel(0, 0)?.let { Integer.toHexString(it) } ?: "-"}")
        return bitmap
    }

    /** 清屏帧判定：清屏帧整幅同色；真实画面几乎不可能全幅同色。
     *  只比 RGB，**不比 alpha**（不透明黑帧 alpha=255，比 alpha 会恒判"有内容"）。 */
    private fun hasContent(bytes: ByteArray): Boolean {
        val r0 = bytes[0]
        val g0 = bytes[1]
        val b0 = bytes[2]
        var i = 0
        val total = WIDTH * HEIGHT
        while (i < total) {
            val o = i * 4
            if (bytes[o] != r0 || bytes[o + 1] != g0 || bytes[o + 2] != b0) return true
            i += 337
        }
        return false
    }

    private fun bitmapFromRgba(bytes: ByteArray): Bitmap? {
        if (bytes.size < WIDTH * HEIGHT * 4) return null
        val bmp = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888)
        bmp.copyPixelsFromBuffer(ByteBuffer.wrap(bytes))
        return bmp
    }

    override fun evidenceJson(): String {
        val latch = java.util.concurrent.CountDownLatch(1)
        var snapshot = ""
        handler.post {
            snapshot = buildEvidenceSnapshot()
            latch.countDown()
        }
        latch.await(2, java.util.concurrent.TimeUnit.SECONDS)
        return snapshot.ifBlank { """{"error":"evidence snapshot timeout: handler busy"}""" }
    }

    private fun buildEvidenceSnapshot(): String {
        val warm = warmLatenciesMs.sorted()
        fun pct(p: Double): Long =
            if (warm.isEmpty()) -1 else warm[(p * (warm.size - 1)).toInt().coerceAtMost(warm.size - 1)]
        val json = org.json.JSONObject()
        json.put("requests", requests)
        json.put("cache_hits", cacheHits)
        json.put("decode_failures", decodeFailures)
        json.put("evictions", evictions)
        json.put("cache_size_final", cache.size())
        json.put("cache_max", cacheSize)
        json.put("init_ms", initMs)
        json.put("cold_first_latency_ms", coldFirstLatencyMs)
        json.put("warm_count", warm.size)
        json.put("warm_p50_ms", pct(0.50))
        json.put("warm_p95_ms", pct(0.95))
        json.put("warm_max_ms", if (warm.isEmpty()) -1 else warm.last())
        json.put("frame_seq", framesAcquired)
        json.put("frames_acquired", framesAcquired)
        val grabs = org.json.JSONArray()
        grabLog.forEach { grabs.put(it) }
        json.put("grabs", grabs)
        return json.toString()
    }

    override fun release() {
        closing = true
        handler.post {
            releaseKernel()
            cache.evictAll()
        }
    }

    private fun releaseKernel() {
        runCatching { session?.close() }
        session = null
        registeredId?.let { id -> runCatching { server?.release(id) } }
        registeredId = null
        runCatching { server?.close() }
        server = null
        initialized = false
    }

    companion object {
        private const val DEFAULT_CACHE_SIZE = 50
        private const val PREWARM_DELAY_MS = 1200L
        private const val WIDTH = 480
        private const val HEIGHT = 270
    }
}