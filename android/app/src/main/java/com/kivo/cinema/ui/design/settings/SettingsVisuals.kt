package com.kivo.cinema.ui.design.settings

import androidx.compose.ui.graphics.Color
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置页语义视觉 tokens（268 v3 · Surge-clean）：冷灰底 + 扁平纯白卡 + 淡底彩 glyph chip。
 * 只负责"设置页视觉常量"；行组合在 SettingsRows/SettingsRowParts，骨架在 SettingsScaffold。
 * v3 裁决：拒粉紫脏底、拒内容卡描边/投影、近白卡太糊亦否 ⇒ 底中性冷灰、卡纯白扁平靠明度差分层。
 * 画布色单一来源 [DesignTokens.pageBackground]（首页 resting/资源库/我的/设置共用 #F2F2F7）。
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

    // v3 画布 / 卡 / 中性灰（副标题/节头 ≥4.5:1）
    val canvas = DesignTokens.pageBackground
    val card = Color(0xFFFFFFFF)
    val hairline = Color(0xFFE3E5EA)
    val sub = Color(0xFF6B7280)
    val sec = Color(0xFF7A8089)
    val chev = Color(0xFFC6C9CE)
    val dimTitle = Color(0xFFB9BDC4)
    val navPill = Color(0xFFE8E9ED)

    // 字阶 / 节奏（v3）
    const val fontRootTitle = 32f
    const val fontSection = 13f
    const val fontRow = 16f
    const val fontSub = 12f
    const val cardRadius = 20f
    const val cardGap = 12f
    const val hInset = 16f
    const val dividerInsetChip = 42f

    fun tint(c: Color): Color = c.copy(alpha = chipTintAlpha)
}