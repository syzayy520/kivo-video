package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

/**
 * 面板模糊背景：抓当前视频帧 → 降采样 → 三遍盒式模糊 → `ImageBitmap`。
 *
 * 【为什么走内核截图】视频层是 SurfaceView，`PixelCopy` 在部分机型/PiP 下拿黑帧；
 * `screenshot-to-file` 与截图按钮同一实现，帧精确。
 * 【为什么只抓一次】mpv 截图是同步命令、会抢内核实锁，面板开着期间不重抓。
 */
internal object PanelBackdrop {

    private const val FILE_NAME = "panel-backdrop.jpg"

    /**
     * 同步阻塞，只能在 IO 线程/协程 IO 调度器上调用。失败返回 null（不抛）。
     *
     * 截图落 `cacheDir/panel-backdrop.jpg`，同名覆盖（幂等，不需要清理）。
     */
    fun capture(
        session: MpvPlaybackSession?,
        cacheDir: File,
        targetWidthPx: Int = 300,
    ): ImageBitmap? {
        val mpv = session ?: return fail(cacheDir, "no-session")
        val width = if (targetWidthPx > 0) targetWidthPx else 300
        val file = File(cacheDir, FILE_NAME)
        // 先删旧图：残留的上一张会让"截图失败"看起来像成功（静默降级）。
        file.delete()
        val accepted = runCatching {
            mpv.command("screenshot-to-file", "\"${file.absolutePath}\" video")
        }.getOrDefault(false)
        if (!accepted) return fail(cacheDir, "command-rejected")

        val shot = awaitDecode(file, width)
            ?: return fail(cacheDir, if (file.isFile) "decode-null" else "no-file")
        val w = shot.width
        val h = shot.height
        val pixels = IntArray(w * h)
        shot.getPixels(pixels, 0, w, 0, 0, w, h)
        shot.recycle()
        blurPixels(pixels, w, h, radius = (minOf(w, h) / 24).coerceIn(3, 16))
        val blurred = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        blurred.setPixels(pixels, 0, w, 0, 0, w, h)

        trace(cacheDir, "panel-backdrop ok ${w}x${h}")
        return blurred.asImageBitmap()
    }

    private fun fail(cacheDir: File, reason: String): ImageBitmap? {
        trace(cacheDir, "panel-backdrop fail=$reason")
        return null
    }
}

/**
 * `open` 从 false→true 时启动一次抓帧；抓帧期间返回上一张（首帧为 null）；
 * `open=false` 立即返回 null（面板已关不需要）。
 */
@Composable
internal fun rememberPanelBackdrop(session: MpvPlaybackSession?, open: Boolean): ImageBitmap? {
    val context = LocalContext.current
    val state = remember(session) { mutableStateOf<ImageBitmap?>(null) }
    LaunchedEffect(session, open) {
        if (!open || session == null) return@LaunchedEffect
        val shot = withContext(Dispatchers.IO) { PanelBackdrop.capture(session, context.cacheDir) }
        if (shot != null) state.value = shot
    }
    return if (open) state.value else null
}

/**
 * 截图命令是异步的：命令返回只代表内核收下，文件可能还没落盘，所以有界轮询。
 * 文件出现但 decode 为 null 也继续轮询（可能正在写）。
 */
private fun awaitDecode(file: File, targetWidthPx: Int): Bitmap? {
    val deadline = System.currentTimeMillis() + 1_500L
    while (System.currentTimeMillis() < deadline) {
        if (file.isFile && file.length() > 0L) {
            decodeSample(file, targetWidthPx)?.let { return it }
        }
        try {
            Thread.sleep(25L)
        } catch (e: InterruptedException) {
            Thread.currentThread().interrupt()
            return null
        }
    }
    return null
}

private fun decodeSample(file: File, targetWidthPx: Int): Bitmap? {
    val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
    BitmapFactory.decodeFile(file.absolutePath, bounds)
    if (bounds.outWidth <= 0 || bounds.outHeight <= 0) return null
    var sample = 1
    while (bounds.outWidth / (sample * 2) >= targetWidthPx) sample *= 2
    val opts = BitmapFactory.Options().apply {
        inSampleSize = sample
        inPreferredConfig = Bitmap.Config.ARGB_8888
    }
    return BitmapFactory.decodeFile(file.absolutePath, opts)
}

private fun blurPixels(pixels: IntArray, w: Int, h: Int, radius: Int) {
    val tmp = IntArray(pixels.size)
    repeat(3) {
        blurPass(pixels, tmp, w, h, radius, horizontal = true)
        blurPass(tmp, pixels, w, h, radius, horizontal = false)
    }
}

/** 一遍可分离盒式模糊：滑窗求和，边缘按最近像素夹取。 */
private fun blurPass(src: IntArray, dst: IntArray, w: Int, h: Int, radius: Int, horizontal: Boolean) {
    val lineLen = if (horizontal) w else h
    val lineCount = if (horizontal) h else w
    val stride = if (horizontal) 1 else w
    val baseStep = if (horizontal) w else 1
    val window = radius * 2 + 1
    for (line in 0 until lineCount) {
        val base = line * baseStep
        var sumA = 0
        var sumR = 0
        var sumG = 0
        var sumB = 0
        for (k in -radius..radius) {
            val p = src[base + k.coerceIn(0, lineLen - 1) * stride]
            sumA += p ushr 24 and 0xFF
            sumR += p shr 16 and 0xFF
            sumG += p shr 8 and 0xFF
            sumB += p and 0xFF
        }
        for (x in 0 until lineLen) {
            dst[base + x * stride] =
                ((sumA / window) shl 24) or ((sumR / window) shl 16) or
                ((sumG / window) shl 8) or (sumB / window)
            val out = src[base + (x - radius).coerceIn(0, lineLen - 1) * stride]
            sumA -= out ushr 24 and 0xFF
            sumR -= out shr 16 and 0xFF
            sumG -= out shr 8 and 0xFF
            sumB -= out and 0xFF
            val into = src[base + (x + radius + 1).coerceIn(0, lineLen - 1) * stride]
            sumA += into ushr 24 and 0xFF
            sumR += into shr 16 and 0xFF
            sumG += into shr 8 and 0xFF
            sumB += into and 0xFF
        }
    }
}

/**
 * 与 `smbTrace` 同一落盘点（`files/evidence/p220-status.log`）：MIUI 过滤应用 logcat，
 * 只有落盘才查得到。`smbTrace` 是 Activity 扩展、这里在 IO 线程拿不到该实例，
 * 所以按同样的格式自己写一行。
 */
private fun trace(cacheDir: File, message: String) {
    Log.i(PlayerComposeActivity.TAG, message)
    runCatching {
        val dataDir = cacheDir.parentFile ?: return@runCatching
        val dir = File(dataDir, "files/evidence").apply { mkdirs() }
        File(dir, "p220-status.log").appendText("${System.currentTimeMillis()} $message\n")
    }
}