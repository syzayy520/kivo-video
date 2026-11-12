package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp

/**
 * **锁定态层**（v3 §1.5 / §二，真机实测）。
 *
 * 锁定后全屏只剩左下一枚**开锁** glyph —— 它不是装饰，是唯一的解锁控件：
 * 位置与播放键同槽（锁体中心 x≈65pt、glyph 心 y≈344.8pt），44pt 命中盒。
 * 其余 chrome 全部不画；弹幕/字幕/手势层照常（§二：锁定保留双击与长按）。
 *
 * 【为什么不吃全屏点击】锁定态单击 = "仅唤出挂锁"，事件仍要落到手势层
 * 的双击/长按通道。所以这里只放一个 44pt 的按钮，不做全屏 pointerInput。
 *
 * 【入口是 ◐ 推断】加锁入口没被录到（ strings 里只有
 * `settings.playback.gestures.locked`「锁定时允许长按和双击」）。
 * 我们暂定**长按播放键**加锁（不占用任何已实测的槽位），
 * 已列入设计稿 §六 的 iPhone 补证清单，真机确认后改这一处即可。
 */
@Composable
internal fun PlayerLockLayer(
    visible: Boolean,
    onUnlock: () -> Unit,
    modifier: Modifier = Modifier,
) {
    if (!visible) return
    // align 由调用方在 BoxScope 里给（Modifier.align 出了 BoxScope 就失效）。
    Box(
        modifier = modifier
            .playerSafeArea(WindowInsetsSides.Horizontal)
            .padding(
                start = PlayerTokens.bottomSidePad.dp,
                bottom = PlayerTokens.lockSlotBottomPad.dp,
            )
            .offset(x = -PlayerTokens.playGroupInset.dp)
            .size(PlayerTokens.actionHitBox.dp)
            .semantics { contentDescription = "解除屏幕锁定" }
            .pointerInput(Unit) { detectTapGestures(onTap = { onUnlock() }) },
        contentAlignment = Alignment.Center,
    ) {
        PlayerGlyph(
            kind = "lock-open",
            color = PlayerTokens.textPrimary,
            modifier = Modifier.size(PlayerTokens.lockGlyphSize.dp),
            strokeWidth = 2f,
        )
    }
}