package com.kivo.cinema.thumbnail

import android.graphics.Bitmap
import android.media.Image

/**
 * YUV_420_888 → RGBA → Bitmap（v3.1 自 [ThumbnailReader] 拆出，守单文件 ≤300 行）。
 *
 * 2×2 半采样（缩略图用不到全分辨率）：4× 转换提速；
 * 4:2:0 下 UV 平面可直接按输出坐标索引。
 * 缓存缩到 320px 宽（D1：低分辨率缓存；50 张 × ~230KB ≈ 11MB）。
 */
internal fun yuvToBitmap(img: Image): Bitmap? {
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
    val targetW = THUMB_TARGET_WIDTH
    val targetH = (targetW.toFloat() * height / width).toInt().coerceAtLeast(1)
    val scaled = Bitmap.createScaledBitmap(half, targetW, targetH, true)
    if (scaled != half) half.recycle()
    return scaled
}

private const val THUMB_TARGET_WIDTH = 320