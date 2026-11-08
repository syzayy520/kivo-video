package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import androidx.compose.ui.graphics.Color
import kotlin.math.pow

/** Average the lower artwork in linear light, so dark encoded values do not crush the field. */
object PosterPalette {
    fun ambientOf(bmp: Bitmap): Color {
        val sums = DoubleArray(3)
        var count = 0
        var y = (bmp.height * .85f).toInt()
        fun linear(channel: Int): Double {
            val value = channel / 255.0
            return if (value <= .04045) value / 12.92 else ((value + .055) / 1.055).pow(2.4)
        }
        while (y < bmp.height) {
            var x = 0
            while (x < bmp.width) {
                val c = bmp.getPixel(x, y)
                sums[0] += linear(c shr 16 and 255)
                sums[1] += linear(c shr 8 and 255)
                sums[2] += linear(c and 255)
                count++
                x += 12
            }
            y += 2
        }
        if (count == 0) return DesignTokens.pageBackground
        fun encoded(sum: Double): Float {
            val v = sum / count
            return (if (v <= .0031308) v * 12.92 else 1.055 * v.pow(1 / 2.4) - .055).toFloat()
        }
        return Color(encoded(sums[0]), encoded(sums[1]), encoded(sums[2]))
    }
}
