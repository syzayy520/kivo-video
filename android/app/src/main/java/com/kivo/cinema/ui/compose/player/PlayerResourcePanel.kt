package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * **资源内联展开**的内容（v3 §1.4 形态 11，点底栏 4K 徽章触发）。
 *
 * 实测（逐像素）：轨道行上移到 ≈198pt；
 * 「播放资源:」16pt + chips 行 y223–253（h30 全圆角，选中白描边 1.5 / 未选白 12%）；
 * 源卡 x[63,272] w208 top265 圆角 16 描边白 35%：
 * SDR 20pt bold → [胶片图标] 1.64 GB + [码率图标] 4.72Mbps（14pt 灰）→
 * [绿色播放标] nyamedia（14pt 白）+ 右端绿底白勾圆 20pt。
 *
 * 【实测推翻设计稿的一处】设计稿写"按钮行不动"，实测**按钮行在这一态消失**
 * （底部只剩源卡）。所以 [PlayerBottomBar] 在资源态不画按钮行。
 *
 * 【chips 为什么可能只有一个】chips 是"同一部片的不同版本"（4K / 其他）。
 * 资源库还没有多版本 ⇒ 只列内核真报的那一个。编一个"其他"让用户点了没反应，
 * 比少一个 chip 更糟。
 */
@Composable
internal fun PlayerResourcePanel(
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    modifier: Modifier = Modifier,
) {
    val chips = items.filter { it.section == "chips" }
    val source = items.firstOrNull { it.section == "source" }

    Column(modifier = modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 13.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                text = "播放资源:",
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                maxLines = 1,
            )
            Row(
                modifier = Modifier
                    .padding(start = 12.dp)
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(PlayerTokens.chipGap.dp),
            ) {
                chips.forEach { chip -> ResourceChip(chip = chip, onSelect = { onSelect(chip) }) }
            }
        }

        if (source != null) {
            SourceCard(source = source, modifier = Modifier.padding(top = PlayerTokens.sourceCardTopGap.dp))
        }
    }
}

@Composable
private fun ResourceChip(chip: PlayerOptionItem, onSelect: () -> Unit) {
    val shape = RoundedCornerShape(PlayerTokens.chipHeight.dp / 2)
    Box(
        modifier = Modifier
            .height(PlayerTokens.chipHeight.dp)
            .clip(shape)
            .background(if (chip.selected) Color.Transparent else PlayerTokens.chipSurface)
            .then(
                if (chip.selected) Modifier.border(1.5.dp, PlayerTokens.textPrimary, shape) else Modifier,
            )
            .pointerInput(chip.tag) { detectTapGestures(onTap = { onSelect() }) }
            .padding(horizontal = 18.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = chip.title,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.textButtonSize.sp,
            maxLines = 1,
        )
    }
}

/** 源卡：徽章 / 体积+码率 / 源名+绿勾。meta 用 " · " 分成两段（装配层约定）。 */
@Composable
private fun SourceCard(source: PlayerOptionItem, modifier: Modifier = Modifier) {
    val shape = RoundedCornerShape(PlayerTokens.panelRadius.dp)
    val meta = source.subtitle.split(" · ")
    Column(
        modifier = modifier
            .width(PlayerTokens.sourceCardWidth.dp)
            .clip(shape)
            .border(1.dp, PlayerTokens.sourceCardBorder, shape)
            .background(PlayerTokens.panelSurface)
            .padding(14.dp),
    ) {
        Text(
            text = source.title,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.sourceBadgeSize.sp,
            fontWeight = FontWeight.Bold,
            maxLines = 1,
        )
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.padding(top = 10.dp),
        ) {
            PlayerGlyph(
                kind = "film",
                color = PlayerTokens.textSecondary,
                modifier = Modifier.size(14.dp),
            )
            Text(
                text = meta.getOrElse(0) { "" },
                color = PlayerTokens.textSecondary,
                fontSize = PlayerTokens.sourceMetaSize.sp,
                maxLines = 1,
                modifier = Modifier.padding(start = 6.dp),
            )
            PlayerGlyph(
                kind = "bitrate",
                color = PlayerTokens.textSecondary,
                modifier = Modifier
                    .padding(start = 18.dp)
                    .size(14.dp),
            )
            Text(
                text = meta.getOrElse(1) { "" },
                color = PlayerTokens.textSecondary,
                fontSize = PlayerTokens.sourceMetaSize.sp,
                maxLines = 1,
                modifier = Modifier.padding(start = 6.dp),
            )
        }
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 10.dp),
        ) {
            PlayerGlyph(
                kind = "play",
                color = PlayerTokens.sourceDot,
                modifier = Modifier.size(14.dp),
            )
            Text(
                text = source.value,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.sourceMetaSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 6.dp)
                    .weight(1f),
            )
            Box(
                modifier = Modifier
                    .size(20.dp)
                    .clip(CircleShape)
                    .background(PlayerTokens.sourceDot),
                contentAlignment = Alignment.Center,
            ) {
                PlayerGlyph(
                    kind = "check",
                    color = Color.White,
                    modifier = Modifier.size(10.dp),
                    strokeWidth = 3f,
                )
            }
        }
    }
}