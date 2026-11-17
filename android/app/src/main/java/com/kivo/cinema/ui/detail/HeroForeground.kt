package com.kivo.cinema.ui.detail

import android.graphics.Bitmap
import androidx.compose.ui.graphics.Color

/**
 * 详情 hero 文字前景判定（P0-1 修复）。
 *
 * 旧实现只看 `fieldColor`（海报环境色，通常被暗化），但文字实际画在
 * 「海报图 + field 渐变」的合成层上：浅色海报在文字带仍占主导，
 * 于是出现白字压浅底不可读。
 *
 * 这里按文字带所在位置把海报亮度与 field 亮度加权合成后再判定，
 * 并保留滚动后底色转浅时切回深色前景的行为。
 *
 * 单一职责：只做「文字带亮度采样 + 前景明暗判定」，不改任何视觉尺寸。
 */
internal object HeroForeground {

    /**
     * 渐变在文字带（meta ~492dp、简介 ~530dp）的平均 alpha。
     * DetailBackdrop 的渐变从 250dp 起、640dp 止，stops 为 .55→.45 / .8→.9；
     * 文字带落在 .58–.8 区间，取 0.70 作为加权平均，比按整段中点估算更贴近真实合成。
     */
    private const val GRADIENT_ALPHA = 0.70f

    /** 与 DetailPage 原 chromeDark 判定同阈值，保持未回归时的行为不变。 */
    private const val DARK_THRESHOLD = 0.40f

    /** 采样网格边长：12×12 = 144 点，足够代表区域亮度，开销可忽略。 */
    private const val SAMPLE = 12

    /** 文字带占海报高度区间：从 55% 到 95%。 */
    private const val BAND_START = 0.55f
    private const val BAND_END = 0.95f

    /**
     * 采样海报文字带的感知亮度（0..1）。
     * 必须在 IO 线程调用（内含 144 次 getPixel）。
     */
    fun textBandLuminance(bitmap: Bitmap): Float {
        val w = bitmap.width.coerceAtLeast(1)
        val h = bitmap.height.coerceAtLeast(1)
        var sum = 0f
        var count = 0
        val last = (SAMPLE - 1).toFloat()
        for (gy in 0 until SAMPLE) {
            val y = (h * (BAND_START + (BAND_END - BAND_START) * gy / last))
                .toInt().coerceIn(0, h - 1)
            for (gx in 0 until SAMPLE) {
                val x = (w * gx / last).toInt().coerceIn(0, w - 1)
                val c = Color(bitmap.getPixel(x, y))
                sum += 0.2126f * c.red + 0.7152f * c.green + 0.0722f * c.blue
                count++
            }
        }
        return sum / count.coerceAtLeast(1)
    }

    /**
     * hero 区（未滚动）文字带合成背景亮度。
     * 海报仍占主导，field 色按 GRADIENT_ALPHA 叠加。
     */
    fun heroBandLuminance(posterLuminance: Float, field: Color): Float {
        val fieldLum = 0.2126f * field.red + 0.7152f * field.green + 0.0722f * field.blue
        return posterLuminance * (1f - GRADIENT_ALPHA) + fieldLum * GRADIENT_ALPHA
    }

    /** 合成背景是否偏暗、应使用白色前景。 */
    fun needsLight(effectiveLuminance: Float): Boolean = effectiveLuminance < DARK_THRESHOLD
}
