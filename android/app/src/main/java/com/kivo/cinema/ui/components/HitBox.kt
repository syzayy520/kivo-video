package com.kivo.cinema.ui.components

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/** 统一命中区：把可点内容包进不小于 44dp 的触控盒（报告 §6.1 命中区下限）。 */
@Composable
fun HitBox(
    size: Dp = MinHitTarget,
    contentAlignment: Alignment = Alignment.Center,
    content: @Composable BoxScope.() -> Unit,
) {
    Box(
        modifier = Modifier.size(size.coerceAtLeast(MinHitTarget)),
        contentAlignment = contentAlignment,
        content = content,
    )
}

// 44dp 是 iOS HIG / 报告 §6.1 给出的最小触控边长；令牌无对应项。
private val MinHitTarget = 44.dp