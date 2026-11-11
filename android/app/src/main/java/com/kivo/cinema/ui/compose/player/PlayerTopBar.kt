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
 * **结构照搬 REX，形态按 Android 重画**：
 * · REX 的顶栏完全透明 —— 我们的片源有大量黑场片头，纯透明图标会看不见，
 *   所以加一层 44pt 的淡渐变（黑 0%→28%）。**只加顶栏高度，不遮画面。**
 * · REX 的图标是线框 —— 用本家族的 [PlayerGlyph] 统一画（见 P2）。
 * · REX 顶栏有"主页"入口 —— 我们是单页播放器，**不做**（不是漏做，是场景不同）。
 *
 * @param title 片名。
 * @param speedText 实时网速（空串则不显示）。
 * @param activeAction 当前处于激活态的动作（切换成功后会高亮 1.5s，见 §五.4）。
 * @param onAction 动作点击回调。
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
    ) {        // ---- 背景渐变（只盖顶栏高度，不遮画面）----
        //
        // 【为什么要这一层而 REX 没有】REX 顶栏完全透明，靠图标自身对比度。
        // 我们的片源有大量黑场片头（实测 Shelter 片头就是全黑），
        // 纯透明图标会**整个消失**。所以加一层极淡的渐变（黑 0%→28%）。
        //
        // 【为什么用 Box 而不是 Column】渐变要画在**整个顶栏**上；
        // 如果把 background 挂在内容 Row 上，渐变只覆盖内容高度，
        // 会出现"贴了条黑带"的硬边（第一版实机就是这个毛病）。
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
                // 挖孔安全区（刘海/摄像头在左上角，返回按钮不能压在下面）
                .playerSafeArea()
                .padding(horizontal = 12.dp),
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
                    "aspect" to "画质",
                    "audio" to "音轨",
                    "subtitle" to "字幕",
                    "cast" to "投屏",
                ).forEach { (kind, label) ->
                    val active = activeAction == kind
                    // 【为什么不用 44dp 的 Box 套住】
                    // 第一版每个图标套了个 44dp 触控 Box，而 Row 的 spacedBy 算的是
                    // **Box 之间的**间距 ⇒ 实际图标间距变成 50+44 = 94pt，
                    // 实测散得不成一排（REX 是 50pt）。
                    // 现在：Box 尺寸 = 图标尺寸，触控区靠
                    // minimumInteractiveComponentSize 撑开（不参与布局宽度计算）。
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
