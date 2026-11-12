package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.EaseOut
import androidx.compose.animation.core.spring

/**
 * 全页动效常量的**唯一来源**（v3.1 §五）。
 *
 * 【为什么收成一个文件】丝滑度调校前，180/140/200/150/250 散在五个文件里，
 * 改一处曲线要翻四处；收口后"全页节奏一致"是可核对的，不是口头承诺。
 *
 * | 部件 | 进 | 出 |
 * | --- | --- | --- |
 * | 控制条 | 180 fade+位移 | 140 |
 * | 浮层面板 | spring(0.25,0.8) fade+0.94 缩放 | 150 easeOut |
 * | 面板钻入换内容 | 150 crossfade | 120 |
 * | 轻提示 | 200 fade | 250 |
 * | 轨道变粗 | 150 | 150 |
 * | 预览浮窗 | 150 fade+0.92 缩放 | 150 |
 * | 亮度/音量 HUD | 150 fade | 300（停 [hudHoldMs] 后） |
 *
 * G1 降级（系统"移除动画"）：位移/缩放归零，只留 alpha [reduceMotionMs]。
 * HUD 本就是纯 alpha，天然满足 G1，无需再降级。
 */
internal object PlayerMotion {
    const val barEnterMs = 180
    const val barExitMs = 140
    // REX §三：面板弹入 spring(response 0.25s, damping 0.8)。
    // Compose spring 用刚度：(2π/response)² ≈ 632。
    val panelSpring = spring<Float>(dampingRatio = 0.8f, stiffness = 632f)
    const val panelExitMs = 150
    val panelExitEasing = EaseOut
    const val panelSwapEnterMs = 150
    const val panelSwapExitMs = 120
    const val toastEnterMs = 200
    const val toastExitMs = 250
    const val trackMs = 150
    // 预览浮窗（REX M03-S06：缩略图 α 拖动 0→1 / 松手 1→0，d=0.15s）。
    const val previewEnterMs = 150
    const val previewExitMs = 150
    // 亮度/音量 HUD（REX M03-S03：淡入 0.15 / 停止 1s / 淡出 0.3）。
    const val hudEnterMs = 150
    const val hudExitMs = 300
    const val hudHoldMs = 1000
    const val reduceMotionMs = 150

    val easing = FastOutSlowInEasing
}