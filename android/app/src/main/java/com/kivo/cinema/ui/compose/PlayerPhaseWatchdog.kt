package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.PlayerPhase

/** 起播缓冲的上限：超过这个时间还"在缓冲"就不是在等，而是起不来。 */
internal const val PLAYER_BUFFER_TIMEOUT_MS = 30_000L

/**
 * 缓冲看门狗（272 审计 D10）。
 *
 * 阶段模型上线后，"缓冲"变成了一条**真实可见**的状态 —— 于是它必须有上界：
 * 无限转圈等于把"起不来"说成"还在等"，比报错更糟（用户会一直等下去）。
 *
 * 30 秒的来由：冷启动真值在百毫秒级（P13-A 4K 首帧 66–133ms），
 * SMB/网络建链是秒级；30 秒足够覆盖慢网，又不至于让人以为死机。
 */
internal fun PlayerComposeActivity.armBufferWatchdog() {
    android.os.Handler(mainLooper).postDelayed({
        if (phase is PlayerPhase.Buffering &&
            System.currentTimeMillis() - phaseHolder.enteredMs >= PLAYER_BUFFER_TIMEOUT_MS) {
            phase = PlayerPhase.Failed("起播超时（30 秒）：内核没有就绪，请检查片源或网络后重试")
            controlsVisible = true
        }
    }, PLAYER_BUFFER_TIMEOUT_MS)
}
