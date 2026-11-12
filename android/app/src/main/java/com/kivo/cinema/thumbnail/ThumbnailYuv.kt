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

/** 清屏帧判定：清屏帧整幅同色；真实画面几乎不可能全幅同色。
 *  只比 RGB，**不比 alpha**（不透明黑帧 alpha=255，比 alpha 会恒判"有内容"）。
 *   stride 337 抽样：480×270 全比太慢，抽样足以区分同色与有内容。 */
internal fun rgbaHasContent(bytes: ByteArray, width: Int, height: Int): Boolean {
    val r0 = bytes[0]
    val g0 = bytes[1]
    val b0 = bytes[2]
    var i = 0
    val total = width * height
    while (i < total) {
        val o = i * 4
        if (bytes[o] != r0 || bytes[o + 1] != g0 || bytes[o + 2] != b0) return true
        i += 337
    }
    return false
}

/** 离屏读回的 RGBA（top-down）→ Bitmap；字节数不足返回 null。 */
internal fun rgbaToBitmap(bytes: ByteArray, width: Int, height: Int): Bitmap? {
    if (bytes.size < width * height * 4) return null
    val bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
    bmp.copyPixelsFromBuffer(java.nio.ByteBuffer.wrap(bytes))
    return bmp
}