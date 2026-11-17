package com.kivo.cinema.ui.profile.lists

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.components.MediaCard
import com.kivo.cinema.ui.components.MediaCardVariant
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.WatchEntry
import com.kivo.cinema.ui.profile.thumbs.EntryCover

/**
 * 「在看」计数卡（R2 复刻，基准 `docs/audit/golden/kivo/profile-1.png` 双卡区右卡）。
 *
 * 版式（✅ 实测）：与 [CollectionSection] 同尺寸半宽白卡 178×86dp、圆角 16dp、内 14dp；
 * 大数字「{n}」30sp Bold [DesignTokens.textMain]、4dp、「在看」15sp [DesignTokens.textMain]；
 * 右上「›」字形高 16px≈5.8dp→20sp 字形，中心距卡顶 25dp、距卡右 25dp（= 内 14dp + 28×22dp 命中盒居中）。
 *
 * 有数据态：卡下方 10dp 接横滑 2:3 海报（复用 [MediaCard] 的 Poster 变体 104×156dp），
 * 点击海报走 [onOpen]，卡与「›」走 [onMore]。基线与冻结签名都没给有数据态的参照帧，
 * 因此这里只按「2:3 海报 + 标题 + 副标题」的字面要求实现，不额外加进度角标。
 *
 * 宽度偏差（有意）：同 [CollectionSection]——本卡自适应可用宽度，半宽由编排方在 `Row` 的
 * `weight(1f)` 容器里给出；直接放进 `Column` 会变成满宽卡。
 */
@Composable
fun WatchingSection(
    entries: List<WatchEntry>,
    onOpen: (WatchEntry) -> Unit,
    onMore: () -> Unit,
) {
    Column(Modifier.fillMaxWidth()) {
        CountCard(count = entries.size, onMore = onMore)
        if (entries.isNotEmpty()) {
            Spacer(Modifier.height(DesignTokens.gap.dp))
            PosterRow(entries = entries, onOpen = onOpen)
        }
    }
}

@Composable
private fun CountCard(count: Int, onMore: () -> Unit) {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .clip(CardShape)
            .background(DesignTokens.canvasFill)
            .border(HairlineWidth, DesignTokens.hairline, CardShape)
            .clickable(role = Role.Button, onClick = onMore)
            .padding(CardPad),
    ) {
        Column(Modifier.align(Alignment.TopStart)) {
            Text(
                text = count.toString(),
                fontSize = CountSize.sp,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Spacer(Modifier.height(CountGap))
            Text(text = Label, fontSize = LabelSize.sp, color = DesignTokens.textMain)
        }
        Box(
            modifier = Modifier
                .align(Alignment.TopEnd)
                .size(MoreHitWidth, MoreHitHeight)
                .semantics(mergeDescendants = true) { contentDescription = "查看全部$Label" },
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = Chevron,
                fontSize = ChevronSize.sp,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
                modifier = Modifier.clearAndSetSemantics { },
            )
        }
    }
}

@Composable
private fun PosterRow(entries: List<WatchEntry>, onOpen: (WatchEntry) -> Unit) {
    LazyRow(
        modifier = Modifier.fillMaxWidth(),
        contentPadding = PaddingValues(horizontal = DesignTokens.screenPad.dp),
        horizontalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        items(entries, key = { it.title + it.subtitle }) { entry ->
            MediaCard(
                title = entry.title,
                subtitle = entry.subtitle,
                variant = MediaCardVariant.Poster,
                onClick = { onOpen(entry) },
                media = { EntryCover(entry = entry, aspectRatio = PosterRatio) },
            )
        }
    }
}

private const val Label = "在看"
private const val Chevron = "›"
private const val CountSize = 30f
private const val LabelSize = 15f
private const val ChevronSize = 20f
private const val PosterRatio = 2f / 3f

private val CardPad = 14.dp
private val CountGap = 4.dp
private val MoreHitWidth = 28.dp
private val MoreHitHeight = 22.dp
private val CardShape = RoundedCornerShape(16.dp)
private val HairlineWidth = 1.dp

@Preview(name = "在看卡 · 空", widthDp = 180, showBackground = true)
@Composable
private fun PreviewWatchingEmpty() {
    Box(Modifier.fillMaxWidth().background(DesignTokens.pageBackground).padding(13.dp)) {
        WatchingSection(entries = emptyList(), onOpen = {}, onMore = {})
    }
}

@Preview(name = "在看卡 · 2 条 + 海报行", widthDp = 200, showBackground = true)
@Composable
private fun PreviewWatchingFilled() {
    Box(Modifier.fillMaxWidth().background(DesignTokens.pageBackground).padding(13.dp)) {
        WatchingSection(
            entries = listOf(
                WatchEntry("kivo 演示剧集", "第 3 集 · 75%", posterPath = null, progressPercent = 75),
                WatchEntry("漫长的季节", "第 4 集 · 30%", posterPath = null, progressPercent = 30),
            ),
            onOpen = {},
            onMore = {},
        )
    }
}