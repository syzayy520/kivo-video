package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * **选集整替**（v3 §1.4 形态 12，点底栏"选集"触发）。
 *
 * 实测：头 y217（✓ + 剧名 16 左 77 / →顺序播放 14 右）；
 * 缩略图 y[242,294] h52 w125 pitch137 圆角 8，当前集白描边 2 外扩 3；
 * 第N集 12pt@303、集名 14pt@315（当前 bold）；背景＝模糊视频 + 黑渐变。
 *
 * 【为什么是"整替"而不是浮层】参考里点选集后**轨道行与按钮行都消失**，
 * 底部换成这一条横滑带。所以它由 [PlayerBottomBar] 整体替换底栏内容，
 * 不是盖在底栏上的面板。
 *
 * 【没有剧集列表时画什么】资源库还没接入剧集 ⇒ 保留头部与横滑带的几何，
 * 带内显示**非空原因**（铁律：不许静默降级），而不是编几集出来。
 */
@Composable
internal fun PlayerEpisodeStrip(
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onOrder: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val playable = items.filter { it.enabled }
    Column(
        modifier = modifier
            .fillMaxWidth()
            .playerSafeArea(WindowInsetsSides.Horizontal)
            .padding(horizontal = PlayerTokens.sidePad.dp),
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            PlayerGlyph(
                kind = "check",
                color = PlayerTokens.textPrimary,
                modifier = Modifier.size(14.dp),
                strokeWidth = 2.2f,
            )
            Text(
                text = title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 8.dp)
                    .weight(1f),
            )
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .pointerInput(Unit) { detectTapGestures(onTap = { onOrder() }) }
                    .padding(start = 12.dp),
            ) {
                PlayerGlyph(
                    kind = "next",
                    color = PlayerTokens.textPrimary,
                    modifier = Modifier.size(12.dp),
                )
                Text(
                    text = "顺序播放",
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.episodeTitleSize.sp,
                    maxLines = 1,
                    modifier = Modifier.padding(start = 6.dp),
                )
            }
        }

        if (playable.isEmpty()) {
            val reason = items.firstOrNull()
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(96.dp)
                    .padding(top = 16.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center,
            ) {
                Text(
                    text = reason?.title ?: "暂无选集",
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.panelTitleSize.sp,
                    maxLines = 1,
                )
                Text(
                    text = reason?.subtitle ?: "资源库尚未提供剧集列表",
                    color = PlayerTokens.textSecondary,
                    fontSize = PlayerTokens.episodeIndexSize.sp,
                    maxLines = 2,
                    modifier = Modifier.padding(top = 4.dp),
                )
            }
        } else {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp)
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(
                    (PlayerTokens.episodeThumbPitch - PlayerTokens.episodeThumbWidth).dp,
                ),
            ) {
                playable.forEach { item -> EpisodeThumb(item = item, onSelect = { onSelect(item) }) }
            }
        }
    }
}

/** 一集：缩略图 + 第N集 + 集名（当前集白描边 2pt、集名 bold）。 */
@Composable
private fun EpisodeThumb(item: PlayerOptionItem, onSelect: () -> Unit) {
    val shape = RoundedCornerShape(PlayerTokens.episodeThumbRadius.dp)
    Column(
        modifier = Modifier
            .width(PlayerTokens.episodeThumbWidth.dp)
            .pointerInput(item.tag) { detectTapGestures(onTap = { onSelect() }) },
    ) {
        Box(
            modifier = Modifier
                .size(
                    width = PlayerTokens.episodeThumbWidth.dp,
                    height = PlayerTokens.episodeThumbHeight.dp,
                )
                .clip(shape)
                .background(PlayerTokens.innerCardSurface)
                .then(
                    if (item.selected) {
                        Modifier.border(PlayerTokens.episodeThumbBorder.dp, PlayerTokens.textPrimary, shape)
                    } else {
                        Modifier
                    },
                ),
            contentAlignment = Alignment.Center,
        ) {
            PlayerGlyph(
                kind = "play",
                color = PlayerTokens.textSecondary,
                modifier = Modifier.size(18.dp),
            )
        }
        Text(
            text = item.subtitle,
            color = PlayerTokens.textSecondary,
            fontSize = PlayerTokens.episodeIndexSize.sp,
            maxLines = 1,
            modifier = Modifier.padding(top = 6.dp),
        )
        Text(
            text = item.title,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.episodeTitleSize.sp,
            fontWeight = if (item.selected) FontWeight.Bold else FontWeight.Normal,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.padding(top = 2.dp),
        )
    }
}