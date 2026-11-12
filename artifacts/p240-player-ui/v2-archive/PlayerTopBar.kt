package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页顶栏（player-ui-v2.md §3.1）。
 *
 * ## 结构照搬 REX，形态按 Android 重画
 * · REX 的顶栏**完全透明** —— 我们的片源有大量黑场片头，纯透明图标会整个消失，
 *   所以加一层 44dp 的淡渐变（黑 0%→28%）。**只加顶栏高度，不遮画面。**
 * · REX 的图标是**线框** —— 用本家族的 [PlayerGlyph] 统一画（见 P2）。
 * · REX 顶栏有"主页"入口 —— 我们是单页播放器，**不做**（不是漏做，是场景不同）。
 *
 * ## 图标组：与参考**逐个对应**
 * | 我们 | REX | 含义 |
 * | --- | --- | --- |
 * | `airplay` | AirPlay | 投屏 |
 * | `danmaku` | 圈内"弹" | 弹幕开关 |
 * | `pip` | 画中画 | 画中画 |
 * | `clapper` | 场记板 | 选集（剧集） |
 * | `expand` | 圈内双向箭头 | 画质比例 |
 * | `equalizer` | 圈内波形条 | 均衡器 |
 * | `captions` | 对话气泡 | 字幕 |
 * | `headphones` | 耳机 | 音轨 |
 *
 * 间距实测 50pt（中心距）—— 见 [PlayerTokens.actionGap]。
 */
@Composable
internal fun PlayerTopBar(
    title: String,
    speedText: String,
    activeAction: String?,
    onClose: () -> Unit,
    onAction: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.topBarHeight.dp),
    ) {
        // ---- 背景渐变（只盖顶栏高度，不遮画面）----
        // 用独立的一层：挂在内容 Row 上会只覆盖内容高度，出现"贴了条黑带"的硬边。
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(PlayerTokens.topBarHeight.dp)
                .background(
                    Brush.verticalGradient(
                        listOf(PlayerTokens.topScrimTop, PlayerTokens.topScrimBottom),
                    ),
                ),
        )

        Row(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.CenterStart)
                // 安全区（挖孔 + 侧边手势区）+ 基础留白
                // 【为什么不再贴边】参考的 chrome 全部按安全区内缩 66pt，
                // 实测最右图标右边缘在 778pt（屏宽 844pt）—— 一条都不贴边。
                .playerSafeArea()
                .padding(horizontal = PlayerTokens.topSidePad.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            // ---- 返回 ----
            Box(
                modifier = Modifier
                    .size(PlayerTokens.closeIconSize.dp)
                    .clickable(onClick = onClose),
                contentAlignment = Alignment.Center,
            ) {
                PlayerGlyph(
                    kind = "close",
                    color = PlayerTokens.textPrimary,
                    modifier = Modifier.size(PlayerTokens.closeIconSize.dp),
                )
            }

            // ---- 片名 ----
            //
            // 【为什么 weight(1f) 而不是 weight(1f, fill=false)】
            // `fill=false` 让标题只占**文字实际宽度**，后面的图标组就被顶到中间去了
            // （第一版实机：图标散在画面中央，与参考完全不符）。
            // 标题撑满剩余空间 ⇒ 图标组自然贴右，与 REX 一致。
            Text(
                text = title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.titleSize.sp,
                lineHeight = PlayerTokens.titleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 2.dp)
                    .weight(1f),
            )

            // ---- 右侧：网速 + 动作图标组 ----
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(PlayerTokens.actionGap.dp),
            ) {
                if (speedText.isNotBlank()) {
                    Text(
                        text = speedText,
                        color = PlayerTokens.textPrimary,
                        fontSize = PlayerTokens.speedSize.sp,
                        maxLines = 1,
                        modifier = Modifier.padding(end = 4.dp),
                    )
                }
                listOf(
                    "airplay" to "投屏",
                    "danmaku" to "弹幕",
                    "pip" to "画中画",
                    "clapper" to "选集",
                    "expand" to "画质比例",
                    "equalizer" to "均衡器",
                    "captions" to "字幕",
                    "headphones" to "音轨",
                ).forEach { (kind, label) ->
                    val active = activeAction == kind
                    // 【为什么不用 44dp 的 Box 套住】
                    // 每个图标套 44dp 触控 Box 时，Row 的 spacedBy 算的是
                    // **Box 之间的**间距 ⇒ 实际图标间距变成 50+44 = 94pt，
                    // 实测散得不成一排（参考是 50pt）。
                    Box(
                        modifier = Modifier
                            .size(PlayerTokens.actionIconSize.dp)
                            .clickable { onAction(kind) },
                        contentAlignment = Alignment.Center,
                    ) {
                        PlayerGlyph(
                            kind = kind,
                            color = if (active) PlayerTokens.activeAccent else PlayerTokens.textPrimary,
                            modifier = Modifier.size(PlayerTokens.actionIconSize.dp),
                        )
                    }
                }
            }
        }
    }
}
