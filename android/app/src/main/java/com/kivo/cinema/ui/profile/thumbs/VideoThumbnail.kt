package com.kivo.cinema.ui.profile.thumbs

import android.content.Context
import android.graphics.Bitmap
import android.media.MediaMetadataRetriever
import android.os.Build
import com.kivo.cinema.net.NetDiag
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException

/**
 * 视频首帧缩略图（PROF-02）：无第三方图片库，用平台 [MediaMetadataRetriever] 取一帧。
 *
 * 缓存：`cacheDir/thumbs/<md5(source)>.jpg`（JPEG 80、长边压到 `maxEdgePx`）。
 * 语义固定：返回值是**缓存 jpg 的绝对路径**，失败返回 null —— UI 只认这个契约，
 * 不在这里给占位图。
 *
 * 单条 3 s 超时：`MediaMetadataRetriever` 的取帧是同步 native 调用，没有 timeout 参数，
 * 所以放到守护线程池里 `Future.get(3s)`；超时后放弃该条（不影响后续条目）。
 *
 * 阻塞调用：请在 `Dispatchers.IO` 上调用。
 */
object VideoThumbnails {

    /** 卡片缩略图长边上限：440dpi 下 2:3 卡片 130dp 宽 ≈ 360px，480 留出余量。 */
    const val DEFAULT_MAX_EDGE_PX = 480

    private const val JPEG_QUALITY = 80
    private const val TIMEOUT_MS = 3_000L
    private const val THUMB_DIR = "thumbs"
    private const val THUMB_EXT = ".jpg"
    private const val TMP_EXT = ".tmp"
    private const val FILE_SCHEME = "file://"

    /** 第 1 秒取帧：避开片头黑场，又不至于跳到正片之后。 */
    private const val FRAME_AT_US = 1_000_000L

    private val worker = Executors.newCachedThreadPool { runnable ->
        Thread(runnable, "kivo-thumb").apply { isDaemon = true }
    }

    /**
     * @param source 片源路径（允许 `file://` 前缀）
     * @return 缓存 jpg 绝对路径；取帧/写盘失败为 null
     */
    fun framePath(context: Context, source: String, maxEdgePx: Int = DEFAULT_MAX_EDGE_PX): String? {
        val path = source.removePrefix(FILE_SCHEME)
        if (path.isBlank()) return null
        val dir = File(context.applicationContext.cacheDir, THUMB_DIR)
        if (!dir.isDirectory && !dir.mkdirs()) return null

        val target = File(dir, md5(path) + THUMB_EXT)
        if (target.length() > 0L) return target.absolutePath

        val bitmap = extractFrame(path, maxEdgePx) ?: return null
        return try {
            writeJpeg(dir, target, bitmap)
        } finally {
            bitmap.recycle()
        }
    }

    private fun writeJpeg(dir: File, target: File, bitmap: Bitmap): String? {
        val tmp = File(dir, target.name + TMP_EXT)
        return try {
            FileOutputStream(tmp).use { out ->
                if (!bitmap.compress(Bitmap.CompressFormat.JPEG, JPEG_QUALITY, out)) {
                    tmp.delete()
                    return null
                }
            }
            // 先写 .tmp 再改名：半张 jpg 不能被后续命中当成有效缓存。
            if (!tmp.renameTo(target)) {
                tmp.delete()
                return null
            }
            target.absolutePath
        } catch (e: Exception) {
            tmp.delete()
            NetDiag.log("[thumb] 写缓存失败：${e.javaClass.simpleName}: ${e.message}")
            null
        }
    }

    private fun extractFrame(path: String, maxEdgePx: Int): Bitmap? {
        val future = worker.submit<Bitmap?> { decode(path, maxEdgePx) }
        return try {
            future.get(TIMEOUT_MS, TimeUnit.MILLISECONDS)
        } catch (e: TimeoutException) {
            future.cancel(true)
            NetDiag.log("[thumb] 取帧超时（>${TIMEOUT_MS}ms）：$path")
            null
        } catch (e: Exception) {
            NetDiag.log("[thumb] 取帧失败：${e.javaClass.simpleName}: ${e.message}")
            null
        }
    }

    private fun decode(path: String, maxEdgePx: Int): Bitmap? {
        val retriever = MediaMetadataRetriever()
        return try {
            retriever.setDataSource(path)
            scaledFrame(retriever, maxEdgePx)
        } catch (e: Exception) {
            null
        } finally {
            runCatching { retriever.release() }
        }
    }

    /** API 27+ 由平台负责缩放；26 上退化为取原帧后自行等比缩放（minSdk = 26）。 */
    private fun scaledFrame(retriever: MediaMetadataRetriever, maxEdgePx: Int): Bitmap? {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
            retriever.getScaledFrameAtTime(
                FRAME_AT_US,
                MediaMetadataRetriever.OPTION_CLOSEST_SYNC,
                maxEdgePx,
                maxEdgePx,
            )?.let { return it }
        }
        val raw = retriever.getFrameAtTime(FRAME_AT_US, MediaMetadataRetriever.OPTION_CLOSEST_SYNC)
            ?: return null
        return scaleTo(raw, maxEdgePx)
    }

    private fun scaleTo(src: Bitmap, maxEdgePx: Int): Bitmap {
        val longEdge = maxOf(src.width, src.height)
        if (longEdge <= maxEdgePx || longEdge <= 0) return src
        val scale = maxEdgePx.toFloat() / longEdge
        val width = (src.width * scale).toInt().coerceAtLeast(1)
        val height = (src.height * scale).toInt().coerceAtLeast(1)
        val out = Bitmap.createScaledBitmap(src, width, height, true)
        if (out !== src) src.recycle()
        return out
    }

    private fun md5(value: String): String = MessageDigest.getInstance("MD5")
        .digest(value.toByteArray(Charsets.UTF_8))
        .joinToString("") { byte -> "%02x".format(byte) }
}