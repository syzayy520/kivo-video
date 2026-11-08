package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import androidx.compose.ui.graphics.Color

/**
 * 海报取色纯函数（239 §4-v13；IMG_0082/0138/0139/0140 + IMG_0142 对照播放器实测）。
 *
 * 实证口径：
 * - 环境色 = hero 海报**底缘 10% 行平均色**（Rex 0138 场顶 (28,10,9) ≈ 原图底缘 (30,6,6)；
 *   用户裁定「下面跟随海报底部的颜色而来」——**不做任何遮罩补偿**，灰白海报就是灰白）。
 * - 清晰海报**原色全画**，环境色遮罩盖在它的外层（上层）：海报下部透过遮罩隐约可见、
 *   底边被遮罩实色区完全盖住（IMG_0142 f1/f13 + Rex 0138 同构）——「海报底边一条线」
 *   从机制上不可能出现。
 * - 旧方案的 blur(art×scrim) 背景层与清晰海报羽化已删：双层渐变叠加在 fy 0.5-1.0
 *   互相露底（v8-v11 反复踩坑），遮罩外层方案一层解决。
 */
object PosterPalette {

    /** 海报底缘 10% 行平均色 = 环境色（下缘场与外层遮罩的源色）。 */
    fun ambientOf(bmp: Bitmap): Color {
        val y0 = (bmp.height * 9) / 10
        var r = 0L; var g = 0L; var b = 0L; var n = 0L
        var y = y0
        while (y < bmp.height) {
            var x = 0
            while (x < bmp.width) {
                val c = bmp.getPixel(x, y)
                r += (c shr 16 and 0xFF); g += (c shr 8 and 0xFF); b += (c and 0xFF); n++
                x += 12
            }
            y += 2
        }
        if (n == 0L) return DesignTokens.bgCream
        return Color(
            red = (r / n).toInt() / 255f,
            green = (g / n).toInt() / 255f,
            blue = (b / n).toInt() / 255f,
        )
    }
}
