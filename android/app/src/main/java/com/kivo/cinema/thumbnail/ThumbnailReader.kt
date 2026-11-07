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

    /// LRU 缓存（键 = 秒，值 = 缩略图位图）
    private val cache = object : LruCache<Int, Bitmap>(cacheSize) {
        override fun entryRemoved(evicted: Boolean, key: Int?, oldValue: Bitmap?, newValue: Bitmap?) {
            oldValue?.recycle()
        }
    }

    private var extractor: MediaExtractor? = null
    private var codec: MediaCodec? = null
    private var videoTrackIndex = -1
    private var durationUs = 0L
    private var initialized = false

    private val thread = HandlerThread("thumbnail").apply { start() }
    private val handler = Handler(thread.looper)

    /// 回调：缩略图就绪（在 handler 线程调）
    var onThumbnailReady: ((positionUs: Long, bitmap: Bitmap) -> Unit)? = null

    /**
     * seek 到指定位置并解码缩略图。
     * 线程安全（post 到 handler）；回调在 handler 线程。
     */
    fun seekTo(positionUs: Long) {
        handler.post {
            val key = (positionUs / 1_000_000).toInt()
            cache.get(key)?.let { cached ->
                onThumbnailReady?.invoke(positionUs, cached)
                return@post
            }
            ensureInitialized()
            val bitmap = decodeFrameAt(positionUs)
            if (bitmap != null) {
                cache.put(key, bitmap)
                onThumbnailReady?.invoke(positionUs, bitmap)
            }
            scheduleRelease()
        }
    }

    /// 释放全部资源（handler 线程上安全调）。
    fun release() {
        handler.post {
            releaseCodecInternal()
            cache.evictAll()
        }
    }

    private fun ensureInitialized() {
        if (initialized) return
        extractor = MediaExtractor()
        extractor?.setDataSource(mediaPath)
        var format: MediaFormat? = null
        for (i in 0 until (extractor?.trackCount ?: 0)) {
            val f = extractor?.getTrackFormat(i) ?: continue
            val mime = f.getString(MediaFormat.KEY_MIME) ?: continue
            if (mime.startsWith("video/")) {
                extractor?.selectTrack(i)
                videoTrackIndex = i
                format = f
                durationUs = f.getLong(MediaFormat.KEY_DURATION)
                break
            }
        }
        if (format == null || extractor == null) return
        codec = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME)!!)
        codec?.configure(format, null, null, 0)
        codec?.start()
        initialized = true
    }

    private fun decodeFrameAt(positionUs: Long): Bitmap? {
        val ex = extractor ?: return null
        val mc = codec ?: return null
        ex.seekTo(positionUs, MediaExtractor.SEEK_TO_CLOSEST_SYNC)

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
                        mc.getOutputImage(outIdx)?.let { img ->
                            val bitmap = yuvToBitmap(img)
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
        // YUV_420_888 → RGBA → Bitmap（M1 最简：逐像素转换）
        val width = img.width
        val height = img.height
        val yPlane = img.planes[0]
        val uPlane = img.planes[1]
        val vPlane = img.planes[2]
        val pixels = IntArray(width * height)
        for (y in 0 until height) {
            for (x in 0 until width) {
                val yIdx = y * yPlane.rowStride + x
                val uvIdx = (y / 2) * uPlane.rowStride + (x / 2) * uPlane.pixelStride
                val yVal = yPlane.buffer[yIdx].toInt() and 0xFF
                val uVal = uPlane.buffer[uvIdx].toInt() - 128
                val vVal = vPlane.buffer[uvIdx].toInt() - 128
                val r = (yVal + 1.402f * vVal).toInt().coerceIn(0, 255)
                val g = (yVal - 0.344f * uVal - 0.714f * vVal).toInt().coerceIn(0, 255)
                val b = (yVal + 1.772f * uVal).toInt().coerceIn(0, 255)
                pixels[y * width + x] = (255 shl 24) or (r shl 16) or (g shl 8) or b
            }
        }
        val bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888)
        return bitmap
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
    }
}
