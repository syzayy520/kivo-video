package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * **听视频态**（v3 §1.4 形态 13，真机复测）。
 *
 * 实测推翻旧稿"全屏黑替换控制条"：顶栏（隐 PiP、耳机反白圆）与底栏**都保留**，
 * 只有视频区变成黑底 + 居中耳机 glyph + 两行字。
 *
 * 【为什么必须自绘黑底】真机实测：`vid=no` 只停解码，SurfaceView **留住最后一帧**，
 * 不盖黑底用户看到的是"画面冻住"。黑底不吃手势 —— 点画面仍走它下面的手势层，
 * 控制条在它上面照常可点。
 *
 * 退出＝再点顶栏耳机钮（[PlayerActionRouter.onTopAction] 的 audioMode 分支）。
 */
@Composable
internal fun PlayerAudioOnly(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Color.Black),
        contentAlignment = Alignment.Center,
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier.padding(bottom = 40.dp),
        ) {
            PlayerGlyph(
                kind = "audioMode",
                color = PlayerTokens.textPrimary,
                modifier = Modifier.size(PlayerTokens.audioOnlyGlyphSize.dp),
                strokeWidth = 1.6f,
            )
            Text(
                text = "正在听视频",
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                modifier = Modifier.padding(top = 18.dp),
            )
            Text(
                text = "画面已关闭，锁屏后继续播放",
                color = PlayerTokens.textSecondary,
                fontSize = PlayerTokens.episodeIndexSize.sp,
                maxLines = 1,
                modifier = Modifier.padding(top = 6.dp),
            )
        }
    }
}