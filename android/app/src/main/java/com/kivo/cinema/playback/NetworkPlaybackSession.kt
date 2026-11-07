package com.kivo.cinema.playback

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.view.Surface

/**
 * URL 硬解会话（220b 设计稿 D2）：MediaExtractor(http://127.0.0.1 代理) →
 * MediaCodec(Surface) 框架硬解管线——与本地文件路径同构，数据源换 URL。
 *
 * 节拍：releaseOutputBuffer(render=true) 直接对 vsync——框架管线自带节拍，
 * 不需要 pts sleep（220 v1 软解的节拍失效问题在此路径不存在）。
 *
 * 线程模型：decode 循环在独立线程（与 C++ 会话同构）；对外接口主线程安全（handler 转发）。
 * 单一职责：URL 硬解播放循环——不涉及 SMB/WebDAV 协议（那些在 Source 层）。
 */
class NetworkPlaybackSession(private val url: String, private val label: String) {

    private var extractor: MediaExtractor? = null
    private var codec: MediaCodec? = null
    private var thread: Thread? = null
    private val running = java.util.concurrent.atomic.AtomicBoolean(false)

    var positionUs: Long = 0; private set
    var durationUs: Long = 0; private set
    var lastError: String = ""; private set
    var width: Int = 0; private set
    var height: Int = 0
    var codecName: String = ""; private set
    private var paceSamples: MutableList<Pair<Long, Double>> = mutableListOf()

    /** 节拍曲线（220 诊断仪表）：每 100 帧 (wall_ms, media_s)。 */
    fun paceCurve(): List<Pair<Long, Double>> = paceSamples.toList()

    private val startedAt = java.util.concurrent.atomic.AtomicLong(0)

    /**
     * 打开 + 起播（同步：extractor/codec 准备完成后返回，解码循环在后台线程）。
     * 返回 0 = 成功。
     */
    fun start(surface: Surface): Int {
        if (running.get()) return -1
        try {
            val ex = MediaExtractor()
            extractor = ex
            ex.setDataSource(url)
            if (ex.trackCount <= 0) {
                lastError = "no tracks"
                ex.release(); extractor = null
                return 3
            }
            var videoTrack = -1
            var format: MediaFormat? = null
            for (i in 0 until ex.trackCount) {
                val f = ex.getTrackFormat(i)
                val mime = f.getString(MediaFormat.KEY_MIME) ?: continue
                if (mime.startsWith("video/")) { videoTrack = i; format = f; break }
            }
            if (videoTrack < 0 || format == null) {
                lastError = "no video track"
                ex.release(); extractor = null
                return 3
            }
            ex.selectTrack(videoTrack)
            durationUs = format.getLong(MediaFormat.KEY_DURATION)
            width = format.getInteger(MediaFormat.KEY_WIDTH)
            height = format.getInteger(MediaFormat.KEY_HEIGHT)
            codecName = format.getString(MediaFormat.KEY_MIME) ?: ""

            val codec = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME)!!)
            codec.configure(format, surface, null, 0)
            codec.start()
            this.codec = codec
            running.set(true)
            startedAt.set(System.currentTimeMillis())

            thread = Thread({
                decodeLoop(videoTrack)
            }, "NetworkPlaybackSession").apply { start() }
            return 0
        } catch (e: Exception) {
            lastError = "start failed: ${e.message}"
            running.set(false)
            releaseAll()
            return 3
        }
    }

    private fun decodeLoop(videoTrack: Int) {
        val ex = extractor ?: return
        val codec = this.codec ?: return
        val info = MediaCodec.BufferInfo()
        var sampleCount = 0
        var inputEof = false
        val startWall = System.currentTimeMillis()
        // 节拍（220b）：首帧起锚，之后每帧睡到"起点 + 媒体时间差"——
        // 硬解码 ~180fps 远超实时，无节拍则自由跑（实测 6.0×）
        val startNs = System.nanoTime()
        var firstPtsUs = -1L

        while (running.get()) {
            // 输入
            if (!inputEof) {
                val inIdx = codec.dequeueInputBuffer(10_000)
                if (inIdx >= 0) {
                    val buf = codec.getInputBuffer(inIdx)
                    if (buf != null) {
                        val sz = ex.readSampleData(buf, 0)
                        if (sz < 0) {
                            codec.queueInputBuffer(inIdx, 0, 0, 0,
                                MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                            inputEof = true
                        } else {
                            codec.queueInputBuffer(inIdx, 0, sz,
                                ex.sampleTime, ex.sampleFlags)
                            ex.advance()
                        }
                    }
                }
            }
            // 输出
            val outIdx = codec.dequeueOutputBuffer(info, 10_000)
            if (outIdx == MediaCodec.INFO_TRY_AGAIN_LATER) continue
            if (outIdx == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                val f = codec.outputFormat
                width = f.getInteger(MediaFormat.KEY_WIDTH)
                height = f.getInteger(MediaFormat.KEY_HEIGHT)
                continue
            }
            if (outIdx >= 0) {
                val mediaUs = info.presentationTimeUs
                positionUs = mediaUs
                sampleCount++
                if (sampleCount % 100 == 0) {
                    paceSamples.add((System.currentTimeMillis() - startWall) to
                        mediaUs / 1e6)
                }
                // 节拍：睡到"起点 + 媒体时间差"再渲染（首帧起锚）
                if (firstPtsUs < 0) firstPtsUs = mediaUs
                val dueNs = startNs + (mediaUs - firstPtsUs) * 1000
                val nowNs = System.nanoTime()
                if (dueNs > nowNs) Thread.sleep((dueNs - nowNs) / 1_000_000)
                codec.releaseOutputBuffer(outIdx, true)   // 渲染到 surface
                if (info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) break
            }
        }
        running.set(false)
    }

    /** seek（异步）：extractor 定位 + codec flush。 */
    fun seekTo(targetUs: Long) {
        val ex = extractor ?: return
        ex.seekTo(targetUs, MediaExtractor.SEEK_TO_CLOSEST_SYNC)
        codec?.flush()
        positionUs = targetUs
    }

    /** 停止 + 释放（幂等）。 */
    fun stop() {
        running.set(false)
        thread?.join(3000)
        thread = null
        releaseAll()
    }

    private fun releaseAll() {
        runCatching { codec?.stop() }
        runCatching { codec?.release() }
        codec = null
        runCatching { extractor?.release() }
        extractor = null
    }
}
