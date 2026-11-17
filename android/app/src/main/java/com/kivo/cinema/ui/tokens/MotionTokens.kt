package com.kivo.cinema.ui.tokens

/**
 * 全局动效时长（页面级/壳级）。播放器内的精细时长仍归 `PlayerMotion`，不在此重复。
 * 所有时长单位 = 毫秒；调用点在 Reduce Motion 生效时应取 0（见 `MotionGate`）。
 */
object MotionTokens {
    // 页面/区块入场：竞品动效拆解 §3.M03（淡入 200ms）
    const val revealMs = 200

    // 元素退场：竞品动效拆解 §3.M03（退场快于入场，150ms）
    const val exitMs = 150

    // 纯透明度变化（无位移）：竞品动效拆解 §3.M03（150ms）
    const val fadeMs = 150

    // 亮度/音量 HUD 淡入：项目既有 PlayerMotion.hudEnterMs 已实测值（0.15s）
    const val hudInMs = 150

    // HUD 峰值停留：项目既有 PlayerMotion.hudHoldMs 已实测值（1s）
    const val hudHoldMs = 1000

    // HUD 淡出：项目既有 PlayerMotion.hudExitMs 已实测值（0.3s）
    const val hudOutMs = 300

    // 主 Tab 页面切换：竞品动效拆解 §3.M03（300ms）
    const val pageTransitionMs = 300
}