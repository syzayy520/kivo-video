package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页字幕层（player-ui-v2.md §3.3）。
 *
 * **它为什么独立于底栏**：底栏（进度条 + 按钮行）是**固定 96pt 高**的，
 * 字幕必须落在它**上面**的 24pt 安全区里。
 * 早先把字幕塞在底栏内部，结果两件事同时坏：
 * ① 底栏被字幕顶高，吃掉画面；② 控制条可能压住字幕
 * （那个 bug 真发生过，见 p6-2-subtitle-diagnosis.md）。
 *
 * **与内核字幕的分工**：mpv 自己画的字幕（PGS 位图）**已经**在正确位置
 * （画面底部、距底 24pt，真机截图验证过）。本层只负责**我们自己的**字幕源
 * （例如外挂 SRT 的 Compose 侧叠加）。两者同源时**只应有一个**显示 ——
 * 那是调用方的责任，本层不判断。
 */
@Composable
internal fun PlayerSubtitleLayer(
    text: String,
    modifier: Modifier = Modifier,
) {
    if (text.isBlank()) return
    Box(
        modifier = modifier.fillMaxWidth(),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = text,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.subtitleSize.sp,
            textAlign = TextAlign.Center,
            maxLines = 2,
            modifier = Modifier
                // 限制宽度：超长字幕不铺满整屏（参考的字幕也不满宽）。
                .widthIn(max = 720.dp)
                .padding(horizontal = 40.dp),
        )
    }
}
