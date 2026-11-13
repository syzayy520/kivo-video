package com.kivo.cinema.ui.design.settings

import androidx.compose.ui.graphics.Color

/**
 * 设置页语义视觉 tokens（268 重设计）：淡底彩 glyph chip 的色板与字阶/间距。
 * 只负责"设置页视觉常量"，行组合在 SettingsRows/SettingsRowParts，骨架在 SettingsScaffold。
 * 色板取 iOS 语义色（参考 EplayerX/SenPlayer/Vidora），淡底 alpha 统一 [chipTintAlpha]。
 */
internal object SettingsVisuals {
    // chip：同色淡底 + 彩色线 glyph
    const val chipTintAlpha = 0.14f
    const val chipSize = 30f
    const val chipRadius = 9f
    const val chipGlyph = 17f

    // 域色（root 入口 / 子页行共用）
    val blue = Color(0xFF0A84FF)
    val green = Color(0xFF34C759)
    val orange = Color(0xFFFF9500)
    val purple = Color(0xFFAF52DE)
    val indigo = Color(0xFF5856D6)
    val pink = Color(0xFFFF2D55)
    val teal = Color(0xFF30B0C7)
    val gray = Color(0xFF8E8E93)

    // 字阶 / 节奏（视觉稿 v2）
    const val fontRootTitle = 32f
    const val fontSection = 12.5f
    const val fontRow = 17f
    const val fontSub = 11f
    const val cardGap = 12f
    const val hInset = 14f

    fun tint(c: Color): Color = c.copy(alpha = chipTintAlpha)
}