package com.kivo.cinema.thumbnail

import android.graphics.Bitmap
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Handler
import android.os.HandlerThread
import android.util.LruCache

/**
 * 独立缩略图读取器（215）：低分辨率 MediaCodec 实例 + LRU 缓存。
 *
 * 与主播放会话**完全解耦**（设计稿 D1）：独立 MediaExtractor + MediaCodec
 * 打开同一文件，seek 独立——主链路零干扰。
 *
 * 懒初始化（D3）：首次 [seekTo] 才开解码器；5 秒无请求释放。
 *
 * 线程模型：全部操作在 [HandlerThread] 上串行执行（MediaCodec 非线程安全）。
 * 公开 API 线程安全（post 到 handler）。
 *
 * LRU 缓存（D2）：键 = 秒级位置（同秒复用），容量 50 张。
 */
class ThumbnailReader(
    private val mediaPath: String,
    private val cacheSize: Int = DEFAULT_CACHE_SIZE
) {

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
    var onThumbnailReady: ((positionUs: Long, bitmap: Bitmap) -> Unit)? = null

    // ---- 取证计数（AC1 / AC3）：全部在 handler 线程读写 ----
    private var requests = 0
    private var cacheHits = 0
    private var decodeFailures = 0
    private var evictions = 0
    private var coldFirstLatencyMs = -1L          // 首次请求（含解码器 init）——AC1 主判据
    private val warmLatenciesMs = ArrayList<Long>()

    /**
     * seek 到指定位置并解码缩略图。
     * 线程安全（post 到 handler）；回调在 handler 线程。
     */
    fun seekTo(positionUs: Long) {
        val requestAt = android.os.SystemClock.elapsedRealtime()
        handler.post {
            requests += 1
            val key = (positionUs / 1_000_000).toInt()
            cache.get(key)?.let { cached ->
                cacheHits += 1
                recordLatency(requestAt)
                onThumbnailReady?.invoke(positionUs, cached)
                return@post
            }
            ensureInitialized()
            val bitmap = if (initialized) decodeFrameAt(positionUs) else null
            if (bitmap != null) {
                cache.put(key, bitmap)
                recordLatency(requestAt)
                onThumbnailReady?.invoke(positionUs, bitmap)
            } else {
                decodeFailures += 1
            }
            scheduleRelease()
        }
    }

    private fun recordLatency(requestAtMs: Long) {
        val latency = android.os.SystemClock.elapsedRealtime() - requestAtMs
        if (coldFirstLatencyMs < 0) coldFirstLatencyMs = latency
        else warmLatenciesMs.add(latency)
    }

    /** 取证快照（AC1/AC3 判据数据）。 */
    fun evidenceJson(): String {
        val latch = java.util.concurrent.CountDownLatch(1)
        var snapshot = ""
        handler.post {
            snapshot = buildEvidenceSnapshot()
            latch.countDown()
        }
        latch.await(2, java.util.concurrent.TimeUnit.SECONDS)
        return snapshot
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
    fun release() {
        handler.post {
            releaseCodecInternal()
            cache.evictAll()
        }
    }

    private fun ensureInitialized() {
        if (initialized || initFailed) return
        extractor = MediaExtractor()
        try {
            extractor?.setDataSource(mediaPath)
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
        while (attempts < 60) {
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
                    if (info.size > 0 && !isEos) {
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

    private fun yuvToBitmap(img: android.media.Image): Bitmap? {
        // YUV_420_888 → RGBA → Bitmap。
        // 2×2 半采样（缩略图用不到全分辨率）：4× 转换提速；4:2:0 下 UV 平面可直接按输出坐标索引。
        val width = img.width
        val height = img.height
        val yPlane = img.planes[0]
        val uPlane = img.planes[1]
        val vPlane = img.planes[2]
        val outW = width / 2
        val outH = height / 2
        if (outW <= 0 || outH <= 0) return null
        val pixels = IntArray(outW * outH)
        val yBuf = yPlane.buffer
        val uBuf = uPlane.buffer
        val vBuf = vPlane.buffer
        var outIdx = 0
        for (oy in 0 until outH) {
            val yRow = oy * 2 * yPlane.rowStride
            val uvRow = oy * uPlane.rowStride
            for (ox in 0 until outW) {
                val yVal = yBuf.get(yRow + ox * 2).toInt() and 0xFF
                val uvIdx = uvRow + ox * uPlane.pixelStride
                val uVal = uBuf.get(uvIdx).toInt() - 128
                val vVal = vBuf.get(uvIdx).toInt() - 128
                val r = (yVal + 1.402f * vVal).toInt().coerceIn(0, 255)
                val g = (yVal - 0.344f * uVal - 0.714f * vVal).toInt().coerceIn(0, 255)
                val b = (yVal + 1.772f * uVal).toInt().coerceIn(0, 255)
                pixels[outIdx++] = (255 shl 24) or (r shl 16) or (g shl 8) or b
            }
        }
        val half = Bitmap.createBitmap(pixels, outW, outH, Bitmap.Config.ARGB_8888)
        // 缓存缩到 320px 宽（D1：低分辨率缓存；50 张 × ~230KB ≈ 11MB）
        val targetW = THUMB_TARGET_WIDTH
        val targetH = (targetW.toFloat() * height / width).toInt().coerceAtLeast(1)
        val scaled = Bitmap.createScaledBitmap(half, targetW, targetH, true)
        if (scaled != half) half.recycle()
        return scaled
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
        private const val THUMB_TARGET_WIDTH = 320
    }
}
