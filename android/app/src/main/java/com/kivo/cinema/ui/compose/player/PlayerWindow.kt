package com.kivo.cinema.ui.compose.player

import android.app.Activity
import android.os.Build
import android.view.WindowManager
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.displayCutout
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.unit.LayoutDirection

/**
 * 播放页的**沉浸式全屏**窗口配置（player-ui-v2.md §二 L1 画面层）。
 *
 * ## 为什么必须是全屏
 * 视频播放器的画面应当**铺满整块屏幕**。顶部的状态栏（时间/信号/电量）与
 * 底部的导航条都会切掉画面，而电影是黑底的 ⇒ 那两条白边在观影时非常刺眼
 * （真机截图里底部那条白条尤其明显）。
 *
 * REX 播放时**也是全屏** —— 给我们的录屏里能看到状态栏，
 * 是因为录屏为了展示功能才把系统栏显出来，不是 REX 的播放态。
 *
 * 用户想看系统栏时下滑即可唤出（`BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE`），
 * 不会做成"永久隐藏"—— 否则没法看电量、也没法退出应用。
 *
 * ## 与内容布局的关系
 * 隐藏系统栏只解决"画面铺满"，**不解决"控件别被挖孔挡住"**：
 * 全面屏手机上摄像头/挖孔在左上角，顶栏的返回按钮可能正好压在下面。
 * 所以：**背景铺满（不避让），控件避让（避让挖孔）**。
 *
 * @param activity 宿主 Activity。
 */
fun applyImmersivePlayerWindow(activity: Activity) {
    val window = activity.window

    // 1) 内容铺到系统栏后面（而不是被系统栏挤进中间）
    androidx.core.view.WindowCompat.setDecorFitsSystemWindows(window, false)

    // 2) 隐藏系统栏（状态栏 + 导航栏）
    val controller = androidx.core.view.WindowCompat.getInsetsController(window, window.decorView)
    controller.hide(
        androidx.core.view.WindowInsetsCompat.Type.systemBars() or
            androidx.core.view.WindowInsetsCompat.Type.displayCutout(),
    )
    controller.systemBarsBehavior =
        androidx.core.view.WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE

    // 3) 系统栏图标一律白色（播放页背景是黑的）
    controller.isAppearanceLightStatusBars = false
    controller.isAppearanceLightNavigationBars = false

    // 4) 允许内容画进刘海/挖孔区（画面要满，控制条再避让）
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
        val mode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        window.attributes = window.attributes.apply { layoutInDisplayCutoutMode = mode }
    }
}

/**
 * 给控件加上**安全区**的内边距（系统栏手势区 + 刘海/挖孔）。
 *
 * 【为什么参考是 66pt，我们不是】
 * REX（iOS）的 chrome 全部按 **safe area** 内缩 66pt ——
 * 实测：顶栏 ✕ 左边缘 65.7pt、最右图标右边缘 778pt（844-778=66pt），
 * 底栏同样左右各 66pt（66.7 / 779.7）。
 * 换算到 Android：等价物就是 **刘海/挖孔 + 侧边手势区**，
 * 所以这里用 [WindowInsets.safeDrawing]。
 *
 * 【只用 safeDrawing 的横向 + 顶部】
 * 横向（侧边手势区/挖孔）与顶部（挖孔）在横屏下**位置固定**；
 * 而底部的系统栏 inset 在系统栏**唤出的瞬间**会变 —— 用它会让控件跳一下。
 *
 * @param sides 要避让哪几边。
 */
fun Modifier.playerSafeArea(
    sides: WindowInsetsSides = WindowInsetsSides.Horizontal + WindowInsetsSides.Top,
): Modifier = composed {
    val direction = LocalLayoutDirection.current
    val padding = WindowInsets.safeDrawing.only(sides).asPaddingValues()
    val start = if (direction == LayoutDirection.Ltr) {
        padding.calculateLeftPadding(direction)
    } else {
        padding.calculateRightPadding(direction)
    }
    val top = padding.calculateTopPadding()
    this.then(
        Modifier.padding(
            start = start,
            top = top,
        ),
    )
}
