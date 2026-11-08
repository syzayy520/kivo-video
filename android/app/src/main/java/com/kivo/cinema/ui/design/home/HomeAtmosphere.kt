package com.kivo.cinema.ui.design.home

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.graphics.luminance
import com.kivo.cinema.ui.design.DesignTokens

/** One continuous color field shared by the artwork mask and the whole page. */
object HomeAtmosphere {
    fun background(ambient: Color, scrollDp: Float): Color {
        // A neutral wash preserves the artwork hue without carrying its deepest
        // shadows through the entire content field.
        val softened = lerp(ambient, DesignTokens.pageBackground, .28f)
        return lerp(softened, DesignTokens.pageBackground, (scrollDp / 390f).coerceIn(0f, 1f))
    }

    // Retain light text through the mid-scroll muted field, switch on the light field.
    fun lightForeground(background: Color): Boolean = background.luminance() < .40f
}
