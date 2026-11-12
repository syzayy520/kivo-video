package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.gestures.detectVerticalDragGestures
import androidx.compose.foundation.gestures.detectDragGesturesAfterLongPress
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput

/**
 * 播放页的**手势层**（单击 / 双击 / 上下滑）。
 *
 * ## 为什么它在覆盖层的**最下面**
 * 第一版手势层放在最上面 ⇒ 它把顶栏/底栏的点击**全部吃掉**（实机：按钮全部点不动）。
 * 放在最下面之后：
 * · 点画面 → 手势层（它兜底）
 * · 点顶栏/底栏/面板 → 它们在**上层**，先拿到事件 ⇒ 按钮照常工作 ✓
 *
 * ## 交互链
 * | 手势 | 行为 | 依据 |
 * | --- | --- | --- |
> | 单击画面 | 控制条显隐 / 面板打开时=关掉面板 | "点屏唤出/再点收起" |
 * | 双击左 1/3 | 快退 | 双击热区 |
 * | 双击右 1/3 | 快进 | 双击热区 |
 * | 双击中 1/3 | 播放/暂停 | 双击热区 |
 *
 * 【单击与双击怎么共存】`detectTapGestures` 自带**识别窗**：
 * 单击的回调会等到双击窗口（~300ms）结束才触发 ⇒ 不会"双击先关控制条又双击快进"。
 * 这正是"识别窗 0.3s"那一条。
 */
@Composable
internal fun PlayerGestureLayer(
    onTap: () -> Unit,
    onDoubleTap: (DoubleTapZone) -> Unit,
    locked: Boolean = false,
    onVerticalDragStart: (Float) -> Unit = {},
    onVerticalDrag: (Float) -> Unit = {},
    onVerticalDragEnd: () -> Unit = {},
    onLongPressStart: (Float, Float) -> Unit = { _, _ -> },
    onLongPressDrag: (Float, Float) -> Unit = { _, _ -> },
    onLongPressEnd: () -> Unit = {},
    modifier: Modifier = Modifier,
) {
    // 【为什么必须用 rememberUpdatedState】
    // `pointerInput(Unit)` 的 key 永远是 Unit ⇒ 这个协程**永不重启**，
    // 于是它捕获的是**第一次组合时**的 onTap 闭包。
    // 实机症状：面板打开后点空白处 → 走的是"旧的"分支（收起控制条），
    // 面板**不关**（用户反馈："点开一个再点空白处居然都没有消失"）。
    // 用 rememberUpdatedState 把最新回调带进长生命周期的手势协程。
    val currentOnTap by rememberUpdatedState(onTap)
    val currentOnDoubleTap by rememberUpdatedState(onDoubleTap)
    val currentLocked by rememberUpdatedState(locked)
    val currentDragStart by rememberUpdatedState(onVerticalDragStart)
    val currentDrag by rememberUpdatedState(onVerticalDrag)
    val currentDragEnd by rememberUpdatedState(onVerticalDragEnd)
    val currentLongPressStart by rememberUpdatedState(onLongPressStart)
    val currentLongPressDrag by rememberUpdatedState(onLongPressDrag)
    val currentLongPressEnd by rememberUpdatedState(onLongPressEnd)

    Box(
        modifier = modifier
            .fillMaxSize()
            // 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。
            // 这个 pointerInput **排在 tap 之前**：
            // 拖动一旦发生就把事件消费掉，tap 检测器随之取消（不会误触发"点一下收起控制条"）。
            // 横向拖动（进度条 seek）不在这里处理 —— 进度条在**上层**，
            // 它会先拿到事件并消费，我们的竖向检测因此不会误触发。
            .pointerInput(Unit) {
                detectVerticalDragGestures(
                    // 锁定态竖滑 = ❌（§二）：不消费、不上报，
                    // 但也不让它误变成 tap —— 滑动本来就不是 tap。
                    onDragStart = { offset -> if (!currentLocked) currentDragStart(offset.x) },
                    onDragEnd = { if (!currentLocked) currentDragEnd() },
                    onDragCancel = { if (!currentLocked) currentDragEnd() },
                    onVerticalDrag = { change, dragAmount ->
                        if (currentLocked) return@detectVerticalDragGestures
                        change.consume()
                        currentDrag(dragAmount)
                    },
                )
            }
            .pointerInput(Unit) {
                detectTapGestures(
                    onTap = { currentOnTap() },
                    onDoubleTap = { offset ->
                        val w = size.width.toFloat().coerceAtLeast(1f)
                        val zone = when {
                            offset.x < w * 0.34f -> DoubleTapZone.BACKWARD
                            offset.x > w * 0.66f -> DoubleTapZone.FORWARD
                            else -> DoubleTapZone.TOGGLE_PLAY
                        }
                        currentOnDoubleTap(zone)
                    },
                )
            }
            // 【长按快进】longPress≥0.5s → 标签淡入 → 左右滑调速
            // → 上滑(Δy≈-60pt)锁定 → 松手恢复。
            // 放在**最后**：它与前两个检测器共享同一批事件，
            // 谁先消费谁生效 —— 竖向拖动与长按拖动都消费移动，
            // 彼此互斥；单击/双击不消费移动，所以长按不会打断它们。
            .pointerInput(Unit) {
                detectDragGesturesAfterLongPress(
                    onDragStart = { offset ->
                        currentLongPressStart(offset.x, offset.y)
                    },
                    onDragEnd = { currentLongPressEnd() },
                    onDragCancel = { currentLongPressEnd() },
                    onDrag = { change, dragAmount ->
                        change.consume()
                        currentLongPressDrag(change.position.x, change.position.y)
                    },
                )
            },
    )
}

/** 双击热区（左 / 中 / 右）。 */
internal enum class DoubleTapZone { BACKWARD, TOGGLE_PLAY, FORWARD }

/** 手势闪示图标的底色（半透明黑圆底 + 白图标）。 */
internal val flashScrim = Color(0x99000000)
