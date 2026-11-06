package com.kivo.cinema.subtitle

import android.graphics.Bitmap
import android.os.Handler
import android.os.Looper
import android.util.Log
import com.kivo.cinema.bridge.NativePlayer
import java.io.File

/**
 * 字幕叠加控制器（212）：`(字幕文件, position_us) → 叠加 View`。
 *
 * 与解码会话**完全解耦**（设计稿 D3）：时间源是 `positionUsSupplier`
 * （硬解路径给 `PlaybackSession.positionUs`，软解路径 M1 不接——差异出版）。
 * seek 同步天然成立：position 跳变后下一轮渲染即新时间点（AC3）。
 *
 * 线程模型：全部回调都在创建线程（UI handler）上——LibassRenderer 的
 * 单线程契约（211 教训）由本类保证；JNI 侧 DirectByteBuffer 当帧拷进 Bitmap。
 *
 * 证据计数：[events]、[renderedFrames]、[lastRenderPositionUs] 供收尾出版
 * （`kivo.p212.subtitle/1`）。
 */
class SubtitleOverlayController(
    private val subtitleFile: File,
    private val view: SubtitleOverlayView,
    private val positionUsSupplier: () -> Long,
    private val handler: Handler,
    private val pollIntervalMs: Long = 100L
) {

    private var handle = 0L
    private var events = 0
    private var renderedFrames = 0
    private var lastRenderPositionUs = -1L
    private var running = false
    private var fontCount = 0

    private val pollRunnable = object : Runnable {
        override fun run() {
            if (!running || handle == 0L) {
                return
            }
            val positionUs = positionUsSupplier()
            if (positionUs >= 0) {
                val dims = IntArray(3)
                val buffer = runCatching {
                    NativePlayer.nativeSubtitleRender(handle, (positionUs / 1000).toInt(), dims)
                }.getOrNull()
                if (buffer != null && dims[0] > 0) {
                    val bitmap = Bitmap.createBitmap(dims[0], dims[1], Bitmap.Config.ARGB_8888)
                    buffer.rewind()
                    bitmap.copyPixelsFromBuffer(buffer)
                    view.setOverlayBitmap(bitmap)
                    ++renderedFrames
                    lastRenderPositionUs = positionUs
                } else {
                    view.clear()
                }
            }
            handler.postDelayed(this, pollIntervalMs)
        }
    }

    /** 挑选并注入字体；返回成功注入的数量（0 = 全失败，证据里如实出版）。 */
    fun loadFonts(candidates: List<String>): Int {
        ensureHandle()
        if (handle == 0L) {
            return 0
        }
        for (path in candidates) {
            if (File(path).isFile) {
                val rc = runCatching {
                    NativePlayer.nativeSubtitleAddFont(handle, path)
                }.getOrDefault(-1)
                if (rc == 0) {
                    ++fontCount
                } else {
                    Log.w(TAG, "font injection failed: $path rc=$rc")
                }
            }
        }
        return fontCount
    }

    /** 加载字幕文件；返回事件数（-1 = 失败）。 */
    fun load(): Int {
        ensureHandle()
        if (handle == 0L) {
            return -1
        }
        events = runCatching {
            NativePlayer.nativeSubtitleLoad(handle, subtitleFile.absolutePath)
        }.getOrDefault(-1)
        if (events < 0) {
            Log.w(TAG, "subtitle load failed: ${lastError()}")
        }
        return events
    }

    fun start() {
        if (running || handle == 0L) {
            return
        }
        running = true
        handler.post(pollRunnable)
    }

    /** 停轮询 + 销毁句柄；返回收尾计数快照。 */
    fun stop(): SubtitleStats {
        running = false
        handler.removeCallbacks(pollRunnable)
        view.clear()
        val stats = SubtitleStats(
            events = events,
            renderedFrames = renderedFrames,
            lastRenderPositionUs = lastRenderPositionUs,
            fontCount = fontCount
        )
        if (handle != 0L) {
            runCatching { NativePlayer.nativeSubtitleDestroy(handle) }
            handle = 0L
        }
        return stats
    }

    private fun lastError(): String =
        if (handle == 0L) "handle is null"
        else runCatching { NativePlayer.nativeSubtitleLastError(handle) }.getOrDefault("")

    private fun ensureHandle() {
        if (handle == 0L) {
            handle = runCatching { NativePlayer.nativeSubtitleCreate() }.getOrElse { 0L }
        }
    }

    data class SubtitleStats(
        val events: Int,
        val renderedFrames: Int,
        val lastRenderPositionUs: Long,
        val fontCount: Int
    )

    companion object {
        private const val TAG = "KivoSubtitle"
    }
}
