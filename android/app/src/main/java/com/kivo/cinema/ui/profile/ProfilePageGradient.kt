package com.kivo.cinema.ui.profile

import androidx.compose.ui.graphics.Brush
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 「我的」页背景渐变：直接取竞品实测端点（`DesignPalette.PageGradient.Light`，
 * 0357/0358 采样 #E5E4F4 → #F8F8FD），不再用 lerp 派生。
 *
 * 拆成独立文件是因为它是**视觉底**而不是编排逻辑，页面文件只引用 [ProfilePageBackdrop]。
 * 深色态暂无 isSystemInDarkTheme 分支，先用 Light；`DesignTokens.pageGradientDark.profile*` 已就绪。
 */
internal val ProfilePageBackdrop: Brush = Brush.verticalGradient(
    listOf(DesignTokens.pageGradient.profileStart, DesignTokens.pageGradient.profileEnd),
)