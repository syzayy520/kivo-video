package com.kivo.cinema.ui.components.hit

import androidx.compose.foundation.layout.sizeIn
import androidx.compose.foundation.layout.wrapContentSize
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/** 48dp 命中下限（274 §6.1 / 403 D6）。播放器横屏 44dp 是显式例外，见 `PlayerTokens.actionHitBox`。 */
internal val HitTargetMin = 48.dp

/**
 * D6 · 透明扩命中：视觉件保持原尺寸，命中区至少 [min]。
 *
 * 只给"父容器有空间、视觉件无底色"的控件用（返回键、日历箭头、文字钮）。
 * 有底色的控件（模块 ✕、设置子页返回）要在外部再包一层原尺寸绘制，
 * 否则 `clip/background` 会跟着扩到 48dp，视觉就变了。
 */
internal fun Modifier.hitTarget(min: Dp = HitTargetMin): Modifier =
    sizeIn(minWidth = min, minHeight = min).wrapContentSize(Alignment.Center)
