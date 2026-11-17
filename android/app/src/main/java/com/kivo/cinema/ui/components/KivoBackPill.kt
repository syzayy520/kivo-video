package com.kivo.cinema.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.Role

/**
 * D5 · 非播放器「返回」圆钮的**唯一容器**。
 *
 * 单职责：统一命中、`Role.Button` 与 `contentDescription="返回"`，并为调用方提供圆形底色槽。
 * 视觉参数（尺寸/底色/图标）一律由调用方原样传入 —— UI 已定稿，本组件不改变任何像素。
 *
 * 有意不覆盖：Emby 首页返回键（Surface + 0.5dp 描边，几何语义不同）、播放器 ✕（关闭语义）。
 */
@Composable
internal fun KivoBackPill(
    onClick: () -> Unit,
    background: Color,
    modifier: Modifier = Modifier,
    glyph: @Composable BoxScope.() -> Unit,
) {
    Box(
        modifier
            .clip(CircleShape)
            .background(background)
            .semantics { contentDescription = "返回"; role = Role.Button }
            .clickable { onClick() },
        contentAlignment = Alignment.Center,
        content = glyph,
    )
}
