package com.kivo.cinema.thumbnail

import android.graphics.Bitmap
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Handler
import android.os.HandlerThread
import android.util.LruCache

/**
 * 独立缩略图读取器（215 / v3.1）：低分辨率 MediaCodec 实例 + LRU 缓存。
 *
 * 与主播放会话**完全解耦**（设计稿 D1）：独立 MediaExtractor + MediaCodec
 * 打开同一文件，seek 独立——主链路零干扰。
 * v3.1：输入源双形态（本地路径 / [android.media.MediaDataSource] 网络字节源），
 * SMB/mpv 链路的拖动预览帧由此供给（[ThumbInput] / [ByteSourceMediaDataSource]）。
 *
 * 懒初始化（D3）：首次 [seekTo] 才开解码器；5 秒无请求释放。
 *
 * 线程模型：全部操作在 [HandlerThread] 上串行执行（MediaCodec 非线程安全）。
 * 公开 API 线程安全（post 到 handler）。**latest-wins**：快速拖动时新请求
 * 顶掉排队中的旧请求（只留最新一个），防解码队列堆积成"拖完还在补帧"。
 *
 * LRU 缓存（D2）：键 = 秒级位置（同秒复用），容量 50 张。
 */
class ThumbnailReader private constructor(
    private val input: ThumbInput,
    private val cacheSize: Int,
) : PreviewFrameReader {

    constructor(mediaPath: String, cacheSize: Int = DEFAULT_CACHE_SIZE) :
        this(ThumbInput.Path(mediaPath), cacheSize)

    constructor(dataSource: android.media.MediaDataSource, cacheSize: Int = DEFAULT_CACHE_SIZE) :
        this(ThumbInput.Source(dataSource), cacheSize)

    /// LRU 缓存（键 = 秒，值 = 缩略图位图）。
    /// 不手动 recycle：UI 可能仍持有最新一张，recycle 会炸渲染；容量 50 张由 LRU 兜内存。
    private val cache = object : LruCache<Int, Bitmap>(cacheSize) {
        override fun entryRemoved(evicted: Boolean, key: Int?, oldValue: Bitmap?, newValue: Bitmap?) {
            if (evicted) evictions += 1
        }
    }

    private var extractor: MediaExtractor? = null
    private var codec: MediaCodec? = null
    private var videoTrackIndex = -1
    private var durationUs = 0L
    private var initialized = false
    private var initFailed = false   // 无视频轨/打开失败 → 停止重试，防每次请求重开 extractor

    private val thread = HandlerThread("thumbnail").apply { start() }
    private val handler = Handler(thread.looper)

    /// 回调：缩略图就绪（在 handler 线程调）
    override var onThumbnailReady: ((positionUs: Long, bitmap: Bitmap) -> Unit)? = null
    override var onUnavailable: ((reason: String) -> Unit)? = null

    // ---- 取证计数（AC1 / AC3）：全部在 handler 线程读写 ----
    private var requests = 0
    private var cacheHits = 0
    private var decodeFailures = 0
    private var evictions = 0
    private var coldFirstLatencyMs = -1L          // 首次请求（含解码器 init）——AC1 主判据
    private val warmLatenciesMs = ArrayList<Long>()

    /**
     * seek 到指定位置并解码缩略图。
     * 线程安全；回调在 handler 线程。latest-wins：排队中的旧请求被最新请求顶掉。
     */
    override fun seekTo(positionUs: Long) {
        latestRequestAtMs = android.os.SystemClock.elapsedRealtime()
        latestUs = positionUs
        handler.removeCallbacks(decodeWorker)
        handler.post(decodeWorker)
    }

    @Volatile private var latestUs = -1L
    @Volatile private var latestRequestAtMs = 0L

    private val decodeWorker = Runnable {
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
        val bitmap = if (initialized) decodeFrameAt(positionUs) else null
        if (bitmap != null) {
            cache.put(key, bitmap)
            recordLatency(requestAt)
            onThumbnailReady?.invoke(positionUs, bitmap)
        } else {
            decodeFailures += 1
            onUnavailable?.invoke(
                if (initFailed) "本地解码不可用（无视频轨或容器不支持）"
                else "本地解码失败（关键帧追不到目标时刻）",
            )
        }
        scheduleRelease()
    }

    private fun recordLatency(requestAtMs: Long) {
        val latency = android.os.SystemClock.elapsedRealtime() - requestAtMs
        if (coldFirstLatencyMs < 0) coldFirstLatencyMs = latency
        else warmLatenciesMs.add(latency)
    }

    /** 取证快照（AC1/AC3 判据数据）。 */
    override fun evidenceJson(): String {
        val latch = java.util.concurrent.CountDownLatch(1)
        var snapshot = ""
        handler.post {
            snapshot = buildEvidenceSnapshot()
            latch.countDown()
        }
        latch.await(2, java.util.concurrent.TimeUnit.SECONDS)
        // 解码线程被 SMB 慢读占住时 latch 会超时：空串会让调用方
        // JSONObject("") 抛 JSONException 直接闪退（真机实证 2026-11-12）。
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
        json.put("cold_first_latency_ms", coldFirstLatencyMs)
        json.put("warm_count", warm.size)
        json.put("warm_p50_ms", pct(0.50))
        json.put("warm_p95_ms", pct(0.95))
        json.put("warm_max_ms", if (warm.isEmpty()) -1 else warm.last())
        return json.toString()
    }

    /// 释放全部资源（handler 线程上安全调）。
    override fun release() {
        handler.post {
            releaseCodecInternal()
            cache.evictAll()
        }
    }

    private fun ensureInitialized() {
        if (initialized || initFailed) return
        extractor = MediaExtractor()
        try {
            extractor?.let { input.applyTo(it) }
        } catch (e: Exception) {
            initFailed = true
            extractor?.release(); extractor = null
            return
        }
        var format: MediaFormat? = null
        for (i in 0 until (extractor?.trackCount ?: 0)) {
            val f = extractor?.getTrackFormat(i) ?: continue
            val mime = f.getString(MediaFormat.KEY_MIME) ?: continue
            if (mime.startsWith("video/")) {
                extractor?.selectTrack(i)
                videoTrackIndex = i
                format = f
                if (f.containsKey(MediaFormat.KEY_DURATION)) {
                    durationUs = f.getLong(MediaFormat.KEY_DURATION)
                }
                break
            }
        }
        if (format == null || extractor == null) {
            initFailed = true
            return
        }
        codec = try {
            MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME)!!)
        } catch (e: Exception) {
            initFailed = true
            null
        } ?: return
        try {
            codec?.configure(format, null, null, 0)
            codec?.start()
        } catch (e: Exception) {
            initFailed = true
            codec?.release(); codec = null
            return
        }
        initialized = true
    }

    private fun decodeFrameAt(positionUs: Long): Bitmap? {
        val ex = extractor ?: return null
        val mc = codec ?: return null
        return try {
            decodeFrameAtInner(ex, mc, positionUs)
        } catch (e: Exception) {
            null   // CodecException/IllegalState 等：计为 decode_failure，不杀进程
        }
    }

    private fun decodeFrameAtInner(ex: MediaExtractor, mc: MediaCodec, positionUs: Long): Bitmap? {
        ex.seekTo(positionUs, MediaExtractor.SEEK_TO_CLOSEST_SYNC)
        // 每次换位置先 flush：不清掉上一位置的解码状态，codec 的输出缓冲会被作废
        // （真机实证：Error 0xffffffde at releaseOutputBuffer，见 215 状态稿 §崩溃根因）
        mc.flush()

        val info = MediaCodec.BufferInfo()
        var attempts = 0
        // 追帧时间预算：大 GOP 上追到目标要解码整 GOP（真机 0.3–0.5s/请求），
        // 浮窗因此落后手指半秒。预算用完就交已解出的最近帧（≤目标），滞后封顶。
        val budgetDeadline = android.os.SystemClock.elapsedRealtime() + FRAME_BUDGET_MS
        // 240 ≈ 10s@24fps：大 GOP 片源从关键帧追到目标时刻的上限；
        // 无索引片源 seek 落点差几十秒时追不上 ⇒ 计 failure，不拿旧帧冒充。
        while (attempts < 240) {
            ++attempts
            val inIdx = mc.dequeueInputBuffer(10_000L)
            if (inIdx >= 0) {
                val inBuf = mc.getInputBuffer(inIdx) ?: break
                val sampleSize = ex.readSampleData(inBuf, 0)
                if (sampleSize < 0) {
                    mc.queueInputBuffer(inIdx, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                } else {
                    mc.queueInputBuffer(inIdx, 0, sampleSize, ex.sampleTime, 0)
                    ex.advance()
                }
            }
            when (val outIdx = mc.dequeueOutputBuffer(info, 10_000L)) {
                MediaCodec.INFO_TRY_AGAIN_LATER -> {}
                MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {}
                else -> {
                    val isEos = (info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0
                    // 追到目标时刻才出帧：直接吐关键帧会让预览显示几秒前的场景。
                    val reached = info.size > 0 && !isEos &&
                        info.presentationTimeUs >= positionUs - 500_000
                    val budgetOut = info.size > 0 && !isEos &&
                        info.presentationTimeUs <= positionUs &&
                        android.os.SystemClock.elapsedRealtime() >= budgetDeadline
                    if (reached || budgetOut) {
                        val img = mc.getOutputImage(outIdx)
                        if (img != null) {
                            val bitmap = yuvToBitmap(img)
                            img.close()   // 不 close 会让 codec 缓冲饿死（60 帧扫描必踩）
                            mc.releaseOutputBuffer(outIdx, false)
                            return bitmap
                        }
                    }
                    mc.releaseOutputBuffer(outIdx, false)
                    if (isEos) break
                }
            }
        }
        return null
    }

    private fun releaseCodecInternal() {
        codec?.stop()
        codec?.release()
        codec = null
        extractor?.release()
        extractor = null
        initialized = false
    }

    private fun scheduleRelease() {
        handler.removeCallbacks(releaseRunnable)
        handler.postDelayed(releaseRunnable, RELEASE_DELAY_MS)
    }

    private val releaseRunnable = Runnable {
        if (!initialized) return@Runnable
        releaseCodecInternal()
    }

    companion object {
        private const val DEFAULT_CACHE_SIZE = 50
        private const val RELEASE_DELAY_MS = 5000L
        private const val FRAME_BUDGET_MS = 120L
    }
}
