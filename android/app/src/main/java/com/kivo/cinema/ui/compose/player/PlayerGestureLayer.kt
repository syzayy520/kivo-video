package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
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
 * | 双指捏合 | 缩放画面 1.0–2.0（锁定态 ❌） | `gestures.pinch`（见 [PlayerPinch]） |
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
    /**
     * 双指缩放状态（B4-T3）：由派生方 `rememberPinchScale(locked)` 持有并传进来。
     * 本层只负责**接收手势**，倍数作用在视频容器上（见 [PlayerPinch]）。
     */
    pinch: PinchScaleState,
    /**
     * 317：暂停**滑动**类手势（横滑 seek / 竖滑亮度音量）。
     *
     * 【为什么不能复用 [locked]】`locked` 是"屏幕锁定"，锁定态**仍要能点击**
     * （点画面 = 解除锁定提示）。而长按快进激活时，点击也不能被吃 ——
     * 所以两者必须分开：这里只压滑动，不动点击。
     */
    suppressSwipe: Boolean = false,
    onHorizontalDragStart: () -> Unit = {},
    onHorizontalDrag: (Float) -> Unit = {},
    onHorizontalDragEnd: (Boolean) -> Unit = {},
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
    val currentSuppressSwipe by rememberUpdatedState(suppressSwipe)
    val currentHDragStart by rememberUpdatedState(onHorizontalDragStart)
    val currentHDrag by rememberUpdatedState(onHorizontalDrag)
    val currentHDragEnd by rememberUpdatedState(onHorizontalDragEnd)
    val currentDragStart by rememberUpdatedState(onVerticalDragStart)
    val currentDrag by rememberUpdatedState(onVerticalDrag)
    val currentDragEnd by rememberUpdatedState(onVerticalDragEnd)
    val currentLongPressStart by rememberUpdatedState(onLongPressStart)
    val currentLongPressDrag by rememberUpdatedState(onLongPressDrag)
    val currentLongPressEnd by rememberUpdatedState(onLongPressEnd)

    // 审查 P1-3：单指检测器统一闸门——锁定 / 长按压滑动 / 双指缩放中，都不响应。
    // 读 Compose State（current* 与 pinch.pinching）⇒ pointerInput 协程虽不重组也能取到最新值。
    fun singleFingerBlocked(): Boolean = currentLocked || currentSuppressSwipe || pinch.pinching

    Box(
        modifier = modifier
            // 【双指缩放：放最外层】Modifier 链上外层 = 事件树的祖先 ⇒ 祖先的
            // `PointerEventPass.Initial` 早于下面四个单指检测器的主分发。
            // 于是"双指成立"能先 consume，横滑/竖滑/长按看到 `isConsumed` 自行取消；
            // 而单指（span=0）时它不消费，单击/双击/横滑/竖滑/长按一律照旧。
            .pinchZoom(pinch, currentLocked)
            .fillMaxSize()
            // 【竖向拖动：亮度/音量】左半屏调亮度，右半屏调音量。
            // 这个 pointerInput **排在 tap 之前**：
            // 拖动一旦发生就把事件消费掉，tap 检测器随之取消（不会误触发"点一下收起控制条"）。
            // 横向拖动（进度条 seek）不在这里处理 —— 进度条在**上层**，
            // 它会先拿到事件并消费，我们的竖向检测因此不会误触发。
            // ★317 · 全屏横向拖动 = 快进/快退。
            //
            // 【为什么之前没有】旧实现只在**底部进度条**上处理横向 seek，
            // 画面上横滑没有任何处理 ⇒ 用户左右滑时手难免带竖向分量，
            // 被下面的竖向检测器抢走 ⇒ 变成调亮度/音量（真机反馈）。
            // 现在补上：横向由本检测器接管，竖向仍归亮度/音量 —— 两者按**主方向**互斥。
            //
            // 【为什么松手才 seek】与进度条拖动同一口径（拖动中只算目标，松手才跳），
            // 避免拖动过程中反复 seek 把内核打乱。
            .pointerInput(Unit) {
                detectHorizontalDragGestures(
                    // 审查 P1-3：多指时显式排除，避免一边缩放一边跳进度
                    // （Initial pass consume 跨不到这里的 Main pass）。
                    onDragStart = { if (!singleFingerBlocked()) currentHDragStart() },
                    // end/cancel 只排除 locked/suppressSwipe（与旧行为一致：锁定态不下发 seek），
                    // 但不排除 pinching —— 多指介入时必须清理 HandSeekState，否则 active 残留；
                    // 多指期间从没 drag ⇒ target=起点，commit 只是 seek 回原位（无害）。
                    onDragEnd = { if (!currentLocked && !currentSuppressSwipe) currentHDragEnd(true) },
                    onDragCancel = { if (!currentLocked && !currentSuppressSwipe) currentHDragEnd(false) },
                    onHorizontalDrag = { change, dragAmount ->
                        if (singleFingerBlocked()) return@detectHorizontalDragGestures
                        change.consume()
                        currentHDrag(dragAmount)
                    },
                )
            }
            .pointerInput(Unit) {
                detectVerticalDragGestures(
                    // 锁定态竖滑 = ❌（§二）：不消费、不上报，
                    // 但也不让它误变成 tap —— 滑动本来就不是 tap。
                    onDragStart = { offset -> if (!singleFingerBlocked()) currentDragStart(offset.x) },
                    onDragEnd = { if (!currentLocked && !currentSuppressSwipe) currentDragEnd() },
                    onDragCancel = { if (!currentLocked && !currentSuppressSwipe) currentDragEnd() },
                    onVerticalDrag = { change, dragAmount ->
                        if (singleFingerBlocked()) return@detectVerticalDragGestures
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
                    // 审查 P1-3：多指缩放时不进入长按快进（避免缩放同时改速度）。
                    onDragStart = { offset ->
                        if (pinch.pinching) return@detectDragGesturesAfterLongPress
                        currentLongPressStart(offset.x, offset.y)
                    },
                    onDragEnd = { currentLongPressEnd() },
                    onDragCancel = { currentLongPressEnd() },
                    onDrag = { change, dragAmount ->
                        if (pinch.pinching) return@detectDragGesturesAfterLongPress
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
