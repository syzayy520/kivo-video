package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput

/**
 * 播放页的**手势层**（REX 交互 M03-S01 / M03-S02）。
 *
 * ## 为什么它在覆盖层的**最下面**
 * 第一版手势层放在最上面 ⇒ 它把顶栏/底栏的点击**全部吃掉**（实机：按钮全部点不动）。
 * 放在最下面之后：
 * · 点画面 → 手势层（它兜底）
 * · 点顶栏/底栏/面板 → 它们在**上层**，先拿到事件 ⇒ 按钮照常工作 ✓
 *
 * ## 交互链（对齐 REX）
 * | 手势 | 行为 | 依据 |
 * | --- | --- | --- |
> | 单击画面 | 控制条显隐 / 面板打开时=关掉面板 | REX"点屏唤出/再点收起" |
 * | 双击左 1/3 | 快退 | REX M03-S01 双击热区 |
 * | 双击右 1/3 | 快进 | REX M03-S01 双击热区 |
 * | 双击中 1/3 | 播放/暂停 | REX M03-S01 双击热区 |
 *
 * 【单击与双击怎么共存】`detectTapGestures` 自带**识别窗**：
 * 单击的回调会等到双击窗口（~300ms）结束才触发 ⇒ 不会"双击先关控制条又双击快进"。
 * 这正是 REX 文档里"识别窗 0.3s"那一条。
 */
@Composable
internal fun PlayerGestureLayer(
    onTap: () -> Unit,
    onDoubleTap: (DoubleTapZone) -> Unit,
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

    Box(
        modifier = modifier
            .fillMaxSize()
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
            },
    )
}

/** 双击热区（REX M03-S01：左 / 中 / 右）。 */
internal enum class DoubleTapZone { BACKWARD, TOGGLE_PLAY, FORWARD }

/** 手势闪示图标的底色（半透明黑圆底 + 白图标）。 */
internal val flashScrim = Color(0x99000000)
