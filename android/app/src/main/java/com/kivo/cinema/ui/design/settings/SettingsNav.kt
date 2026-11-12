package com.kivo.cinema.ui.design.settings

import androidx.activity.compose.BackHandler
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.core.EaseOut
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.unit.IntOffset

/** 子页路由（手机单栏）。各子页互不嵌套 ⇒ 返回栈深度 ≤1，用可空当前路由即可，不堆多级栈。 */
internal enum class SettingsRoute { PLAYBACK, AV, DATA, CACHE, ABOUT }

/**
 * push/pop 路由 + 系统返回 + 水平滑入滑出（261 设计 §四）。
 * 时长/缓动取候选值 0.35s 整页平移，真机拟合后定稿——此处为候选值，非实测常量。
 * rememberSaveable：配置变更/进程重建后恢复所在子页（tab 切换走 MainActivity 路由，回设置页为根页）。
 */
@Composable
internal fun SettingsNavHost(
    root: @Composable ((SettingsRoute) -> Unit) -> Unit,
    sub: @Composable (SettingsRoute, () -> Unit) -> Unit,
) {
    var route by rememberSaveable { mutableStateOf<SettingsRoute?>(null) }
    BackHandler(enabled = route != null) { route = null }
    AnimatedContent(
        targetState = route,
        transitionSpec = {
            val forward = targetState != null
            val slideSpec = tween<IntOffset>(350, easing = EaseOut)
            val fadeSpec = tween<Float>(350)
            val enter = if (forward) slideInHorizontally(slideSpec) { it }
            else slideInHorizontally(slideSpec) { -it / 3 }
            val exit = if (forward) slideOutHorizontally(slideSpec) { -it / 3 }
            else slideOutHorizontally(slideSpec) { it }
            (enter + fadeIn(fadeSpec)) togetherWith (exit + fadeOut(fadeSpec))
        },
        label = "settings-nav",
    ) { current ->
        if (current == null) root { route = it }
        else sub(current) { route = null }
    }
}