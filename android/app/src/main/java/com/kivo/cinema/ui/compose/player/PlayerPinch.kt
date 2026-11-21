package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.calculateZoom
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.ui.input.pointer.pointerInput

/**
 * 播放页的**双指缩放**（B4-T3 / 审计 GST-06）。
 *
 * ## 竞品契约
 * `gestures.pinch` ✅ 存在（"双指缩放画面"）；范围 ◐ 1.0–2.0（竞品未给数值，取行业惯例）；
 * 松手回弹 ❌ 未确认 —— 所以**不做回弹**：缩放是连续量，停在用户松手的那一刻，
 * 缩回 1.0 即自动回正（`coerceIn` 的下界就是 1.0，等于归位）。
 *
 * ## 为什么状态由 Composition 持有
 * `rememberPinchScale` 里的 `remember` 让每份播放页组合各持一份：
 * 退出播放页 ⇒ Composition 销毁 ⇒ 下次进来一定是 1.0，不会带着上次的放大倍数。
 * 与 B4-T3 同一诉求（手势状态不进进程级单例）。
 *
 * ## 手势挂在哪
 * [Modifier.pinchZoom] 由 `PlayerGestureLayer` 挂在**手势层 Box 的最外层**（见该文件），
 * 缩放值由 `PlayerOverlay` 传给 `PlayerComposeRoot` 的视频容器（`graphicsLayer`）。
 *
 * 【为什么用 Initial pass 而不是 `detectTransformGestures`】手势层上还有四个单指检测器
 * （横滑 seek / 竖滑亮度音量 / 单击双击 / 长按）。Modifier 链上**外层 = 事件树的祖先**：
 * 祖先的 `PointerEventPass.Initial` 早于后代的主分发，所以在这里先 `consume()` 能让
 * 下游检测器看到 `isConsumed` 并自行取消 —— 双指一旦成立就不会被"A 指横滑"抢成一 seek。
 * 反过来若用 `detectTransformGestures`（只吃主分发），单指 span 为 0 时它仍会消费 pan，
 * 会直接吃掉横竖滑。
 *
 * ## 已知风险（未覆盖项）
 * ◐ 推断：画面层是 `SurfaceView`（零拷贝路径），它由 SurfaceFlinger 独立合成，
 * Compose 侧 `graphicsLayer` 的缩放能否真正作用到视频画面取决于设备与 API 级别，
 * 静态无法确定。真机验证若无效，正解是把倍数写到内核 `video-zoom`（= log2(scale)），
 * 属渲染侧改动，本卡按约束不做（只挂 1 行 modifier）。
 */
class PinchScaleState internal constructor() {

    /** 当前放大倍数；1.0 = 原始大小（[applyZoom] 已夹在 [MIN_SCALE]…[MAX_SCALE]）。 */
    var scale by mutableFloatStateOf(MIN_SCALE)
        private set

    /** 叠乘一次手势 zoom 增量（每帧调用）；夹紧到合法区间，非法值直接忽略。 */
    fun applyZoom(zoom: Float) {
        if (!zoom.isFinite() || zoom <= 0f || zoom == 1f) return
        scale = (scale * zoom).coerceIn(MIN_SCALE, MAX_SCALE)
    }

    /** 回正到 1.0（锁定态、以及将来"切集归位"的入口）。 */
    fun reset() {
        scale = MIN_SCALE
    }

    /** 是否正在双指捏合（`PlayerGestureLayer` 用它判断"别打断当前手势"）。 */
    var pinching by mutableStateOf(false)
        internal set
}

/** 缩放下限：1.0（不缩小；缩小会让 SurfaceView 露出黑底）。 */
internal const val MIN_SCALE = 1f

/** 缩放上限：2.0（◐ 推断：竞品未给值，取双指可及且不糊的常规上限）。 */
internal const val MAX_SCALE = 2f

/**
 * 记住一份缩放状态，并在**锁定态**回正。
 *
 * 【为什么锁定要 reset】锁定矩阵（`gestures.locked`）里双指 = ❌。若只是"锁定期间不响应"，
 * 画面会停在放大态且用户无法缩小（手势被禁）—— 所以锁定时直接把 [PinchScaleState.scale]
 * 归 1.0，视觉与矩阵一致。
 */
@Composable
fun rememberPinchScale(locked: Boolean): PinchScaleState {
    val state = remember { PinchScaleState() }
    LaunchedEffect(locked) { if (locked) state.reset() }
    return state
}

/**
 * 双指缩放手势（挂手势层最外层；`locked` 变化时重建手势协程）。
 *
 * 只在**同时 ≥2 指按下**时接管并消费：单指一律放行，否则单击/双击/横滑/竖滑/长按全部失效。
 * 锁定态**不消费**（手势层下游的锁定判定本就不响应滑动，点击照常可用）。
 */
fun Modifier.pinchZoom(state: PinchScaleState, locked: Boolean): Modifier =
    this.pointerInput(locked) {
        awaitEachGesture {
            awaitFirstDown(requireUnconsumed = false, pass = PointerEventPass.Initial)
            do {
                val event = awaitPointerEvent(PointerEventPass.Initial)
                // ⚠️ `pinching` 必须按**真实指数**置位。之前在 down 时就无条件置 true，
                // 而手势层把 `pinching` 当"双指进行中"的打断信号 ⇒ 单指手势刚 down
                // 就被 cancel，tap/双击/横滑/竖滑/长按**全部失灵**（2026-11-21 真机实证：
                // touchLog probe down 全部 consumed=false、tap 后控制条永不出现）。
                val twoFingers = event.changes.count { it.pressed } >= 2
                state.pinching = twoFingers
                if (twoFingers && !locked) {
                    state.applyZoom(event.calculateZoom())
                    event.changes.forEach { it.consume() }
                }
            } while (event.changes.any { it.pressed })
            state.pinching = false
        }
    }