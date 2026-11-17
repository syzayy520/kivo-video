package com.kivo.cinema.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.Dp
import com.kivo.cinema.ui.components.hit.hitTarget

/**
 * D5/D6 · 非播放器「返回」圆钮的唯一容器。
 *
 * 单职责：统一命中（≥48dp）、`Role.Button` 与 `contentDescription="返回"`，并为调用方提供圆形底色槽。
 * 视觉参数（[visualSize] 直径、[background] 底色、图标槽）一律由调用方原样传入 —— UI 已定稿，不改任何像素；
 * 命中盒用透明层扩到 48dp，圆本身仍是 [visualSize]。
 *
 * 有意不覆盖：Emby 首页返回键（Surface + 0.5dp 描边，几何语义不同，只补语义）、播放器 ✕（关闭语义）。
 */
@Composable
internal fun KivoBackPill(
    onClick: () -> Unit,
    background: Color,
    visualSize: Dp,
    modifier: Modifier = Modifier,
    glyph: @Composable BoxScope.() -> Unit,
) {
    Box(
        modifier
            .hitTarget()
            .semantics { contentDescription = "返回"; role = Role.Button }
            .clickable(role = Role.Button) { onClick() },
        contentAlignment = Alignment.Center,
    ) {
        Box(
            Modifier.size(visualSize).clip(CircleShape).background(background),
            contentAlignment = Alignment.Center,
            content = glyph,
        )
    }
}
