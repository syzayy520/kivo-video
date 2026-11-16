package com.kivo.cinema.ui.design.home

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.graphics.luminance
import com.kivo.cinema.ui.design.DesignTokens

/** One continuous color field shared by the artwork mask and the whole page. */
object HomeAtmosphere {
    /**
     * @param wash 向页面底色柔化的比例。首页用 .28（浅色 App 的柔和风格）；
     *   服务器首页用更小的值（更贴近海报本色、整体更暗），因为那里是"海报墙"语境，
     *   太亮的底会让白字压不住、也让海报显得不突出。
     */
    fun background(ambient: Color, scrollDp: Float, wash: Float = .28f): Color {
        // A neutral wash preserves the artwork hue without carrying its deepest
        // shadows through the entire content field.
        val softened = lerp(ambient, DesignTokens.pageBackground, wash)
        return lerp(softened, DesignTokens.pageBackground, (scrollDp / 390f).coerceIn(0f, 1f))
    }

    // Retain light text through the mid-scroll muted field, switch on the light field.
    fun lightForeground(background: Color): Boolean = background.luminance() < .40f
}
