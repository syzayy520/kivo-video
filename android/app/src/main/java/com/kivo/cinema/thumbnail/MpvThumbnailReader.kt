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

/**
 * 网络/无索引源使用独立 mpv 实例：MediaExtractor 缺 MKV Cues 时会线性扫全片。
 * vo=libmpv + 离屏 FBO 读回，关键帧 seek（不按 PTS 严格对齐）；Adreno 的
 * ImageReader 传输曾实测损坏，因此保持 FBO 路径与 native 帧代数去重。
 * 首帧后调用 [prewarm]，再延后 1.2s；内核随会话常驻，秒级 LRU 复用。
 */
class MpvThumbnailReader(
    private val appContext: Context,
    private val input: ByteSource,
    private val cacheDir: File,
    private val cacheSize: Int = DEFAULT_CACHE_SIZE,
    /** 取证用的追加 mpv 选项（生产路径为空）。见 [MpvThumbOptions.apply]。 */
    private val extraMpvOptions: List<Pair<String, String>> = emptyList(),
    /** 274 实验响应上限（字节），由 Activity 传入；0 使用默认 2 MiB。 */
    private val maxRangeBytesOverride: Long = 0L,
) : PreviewFrameReader {

    private val cache = object : LruCache<Int, Bitmap>(cacheSize) {
        override fun entryRemoved(evicted: Boolean, key: Int?, oldValue: Bitmap?, newValue: Bitmap?) {
            if (evicted) evictions += 1
        }
    }

    private val thread = HandlerThread("mpv-thumb").apply { start() }
    private val handler = Handler(thread.looper)

    @Volatile override var onThumbnailReady: ((request: PreviewRequest, bitmap: Bitmap) -> Unit)? = null
    @Volatile override var onUnavailable: ((request: PreviewRequest, reason: String) -> Unit)? = null

    private var server: LocalRangeServer? = null
    private var registeredId: String? = null
    private var session: MpvSession? = null
    private var initialized = false
    private var initFailed = false
    private var initRetries = 0
    @Volatile private var closing = false
    private var unavailableReason = "预览帧不可用"
    private val requestGate = PreviewRequestGate()
    private var decodeWorker: Runnable? = null // 仅在 requestGate 锁内访问。

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
        synchronized(requestGate) {
            if (closing) return
            val request = requestGate.next(positionUs)
            val requestAt = SystemClock.elapsedRealtime()
            decodeWorker?.let { handler.removeCallbacks(it) }
            decodeWorker = Runnable { decode(request, requestAt) }.also { handler.post(it) }
        }
    }

    override fun invalidate() {
        synchronized(requestGate) {
            requestGate.invalidate()
            decodeWorker?.let { handler.removeCallbacks(it) }
            decodeWorker = null
        }
    }

    override fun isCurrent(request: PreviewRequest): Boolean = !closing && requestGate.isCurrent(request)

    override fun prewarm() {
        if (closing) return
        // 让主内核先完成起播读盘：两条 SMB 读路径同时抢链路会拖慢首帧。
        handler.postDelayed({ if (!closing) ensureInitialized() }, PREWARM_DELAY_MS)
    }

    private fun decode(request: PreviewRequest, requestAt: Long) {
        if (!isCurrent(request)) return
        requests += 1
        val key = (request.positionUs / 1_000_000).toInt()
        cache.get(key)?.let { cached ->
            cacheHits += 1
            recordLatency(requestAt)
            if (isCurrent(request)) onThumbnailReady?.invoke(request, cached)
            return
        }
        // 冷失败一次自动重建：保留原有有界重试策略。
        if (initFailed && initRetries < MAX_INIT_RETRIES) {
            initFailed = false
            initRetries += 1
        }
        ensureInitialized()
        if (!isCurrent(request)) return
        val bitmap = if (initialized) grabFrameAt(request.positionUs) else null
        if (bitmap != null) {
            cache.put(key, bitmap)
            recordLatency(requestAt)
            if (isCurrent(request)) onThumbnailReady?.invoke(request, bitmap)
        } else {
            decodeFailures += 1
            if (isCurrent(request)) onUnavailable?.invoke(request, unavailableReason)
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
            val proxy = LocalRangeServer.start(
                maxRangeBytes = if (maxRangeBytesOverride > 0L) maxRangeBytesOverride
                else DEFAULT_PREVIEW_MAX_RANGE_BYTES,
            )
            server = proxy
            val id = proxy.register(input)
            registeredId = id
            val native = MpvSession.create(appContext)
            session = native
            native.initialize(
                vo = "libmpv",
                ao = "",
                logFile = File(cacheDir, "mpv-thumb.log").absolutePath,
                // 不给字体目录：预览内核不渲染字幕（sid=no），
                // libass 从不初始化 ⇒ 一个字体都不该加载
                // （真机日志：预览内核 0 条 "Loading font file"）。
            )
            native.setProperty("pause", "yes")
            // 预览内核的其余选项（关键帧 seek / 不出声 / 预读上限）集中在一处设，
            // 并带回执：设不动的那几项必须留痕，否则"限流参数配了但没生效"查不出来。
            val optFailures = MpvThumbOptions.apply(native, extraMpvOptions)
            if (optFailures.isNotEmpty()) {
                File(cacheDir, "mpv-thumb.log").appendText("OPTFAIL ${optFailures.joinToString(",")}\n")
            }
            native.attachCapture(WIDTH, HEIGHT)
            native.loadFile(proxy.urlFor(id, reconnect = false))
            val deadline = System.currentTimeMillis() + LOAD_DEADLINE_MS
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
            // 排空旧帧后记代数：只接受代数 > gen0 的帧（= 本次 seek 之后渲染的）。
            // 不能用 pts 差对齐：关键帧 seek 常连续落在同一关键帧，pts 不变，
            // 新帧会被误判成旧帧整 6s 超时（真机：首次拖动浮窗恒"准备中"）。
            // readCaptureFrame 在 native 已闸掉 seek 挂起期的旧帧重绘
            // （SMB 大文件上旧帧盖新代数被误收 → 浮窗冻帧的根因）。
            native.readCaptureFrame()
            val gen0 = native.captureFrameGen()
            native.seek(target, exact = false) // 关键帧口径：hr-seek=no 已禁前向解码。
            val frameDeadline = startMs + 6_000
            // 真黑场（片头黑底标题）整幅同色，hasContent 恒 false；2s 后接受同色帧兜底。
            val uniformFallbackMs = startMs + 2_000
            var bytes: ByteArray? = null
            var lastUniform: ByteArray? = null
            while (System.currentTimeMillis() < frameDeadline) {
                val candidate = native.readCaptureFrame()
                if (candidate != null && native.captureFrameGen() > gen0) {
                    if (rgbaHasContent(candidate, WIDTH, HEIGHT)) {
                        bytes = candidate
                        break
                    }
                    lastUniform = candidate // 黑场帧只出现一次，留存供兜底而非再读。
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
                bitmap = rgbaToBitmap(bytes, WIDTH, HEIGHT)
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
        json.put("init_retries", initRetries)
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
        synchronized(requestGate) {
            if (closing) return
            closing = true
            invalidate()
            handler.post {
                releaseKernel()
                cache.evictAll()
            }
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
        // 对齐主内核起播预算（MpvPlaybackSession 30s）：慢链路 mp4 moov 读取 15s 不够（257 R-3）。
        private const val LOAD_DEADLINE_MS = 30_000L
        private const val MAX_INIT_RETRIES = 1
        private const val WIDTH = 480
        private const val HEIGHT = 270

        // 273 历史响应上限；实际默认值见文件末尾的 274 实验值。
        private const val PREVIEW_MAX_RANGE_BYTES = 4L * 1024 * 1024
    }
}

/** 274 装置实验：2 MiB 容纳 0.66–1.8 MB 关键帧簇，避免旧 4 MiB 响应的慢链路等待。 */
private const val DEFAULT_PREVIEW_MAX_RANGE_BYTES = 2L * 1024 * 1024