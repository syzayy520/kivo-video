package com.kivo.cinema.ui.profile.lists

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.components.SectionHeader
import com.kivo.cinema.ui.components.SectionLevel
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.WatchEntry
import com.kivo.cinema.ui.profile.thumbs.EntryCover

/**
 * 「待看」区（R2 复刻，基准 `docs/audit/golden/kivo/profile-1.png` 待看区）。
 *
 * 版式（✅ 逐像素实测）：标题行「待看 {n}」16sp Bold + 右侧「›」（40px=14.5dp 字形，命中盒 28×22dp
 * 来自 `SectionHeader`）；标题行与白卡间距 10dp；空态白卡左右 13dp、圆角 16dp（44px）、
 * 上下内边距 22dp、卡高 123dp（338px）。
 *
 * 空态（基准实测）：居中票根图标 32×24dp（描边 1.5dp、[DesignTokens.textSub]）+ 12dp +
 * 「还没有待看的影片」16sp Bold [DesignTokens.textMain] + 4dp +「在详情页加入待看，票根会出现在这里」
 * 13sp [DesignTokens.textSub]（实测 #8A8A8E = textSub）。
 *
 * 有数据态（基线与冻结签名都未给参照）：两列票根卡，每卡 16:9 封面 + 标题 15sp Bold + 副标题 13sp。
 * 规则：有数据时票根卡替代空态卡，不同时出现（避免同屏两条「待看」语义）。
 *
 * 未覆盖：票根卡的进度/集数角标、长列表滚动（这里用 fixed grid，条数上限由数据层控制）。
 */
@Composable
fun WatchlistSection(
    entries: List<WatchEntry>,
    onOpen: (WatchEntry) -> Unit,
    onAdd: () -> Unit,
) {
    Column(Modifier.fillMaxWidth()) {
        SectionHeader(
            title = "$SectionTitle ${entries.size}",
            level = SectionLevel.Secondary,
            onMore = onAdd,
        )
        Spacer(Modifier.height(DesignTokens.gap.dp))
        if (entries.isEmpty()) EmptyTicketCard() else TicketGrid(entries, onOpen)
    }
}

@Composable
private fun EmptyTicketCard() {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(CardShape)
            .background(DesignTokens.canvasFill)
            .border(HairlineWidth, DesignTokens.hairline, CardShape)
            .padding(vertical = EmptyCardPad, horizontal = DesignTokens.screenPad.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        TicketGlyph()
        Spacer(Modifier.height(GlyphGap))
        Text(
            text = EmptyTitle,
            fontSize = EmptyTitleSize.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        Spacer(Modifier.height(EmptyTitleGap))
        Text(
            text = EmptyMessage,
            fontSize = EmptyMessageSize.sp,
            color = DesignTokens.textSub,
        )
    }
}

/** 票根图标：圆角矩形轮廓 + 两条场次横线，纯 Canvas 绘制（不引入图标资源）。 */
@Composable
private fun TicketGlyph() {
    Canvas(Modifier.size(GlyphWidth, GlyphHeight)) {
        val stroke = GlyphStroke.toPx()
        val ink = DesignTokens.textSub
        drawRoundRect(
            color = ink,
            topLeft = Offset(stroke / 2f, stroke / 2f),
            size = Size(size.width - stroke, size.height - stroke),
            cornerRadius = CornerRadius(GlyphCorner.toPx()),
            style = Stroke(width = stroke),
        )
        val begin = size.width * LineBegin
        drawLine(
            color = ink,
            start = Offset(begin, size.height * LineTopY),
            end = Offset(size.width * LineFullEnd, size.height * LineTopY),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
        drawLine(
            color = ink,
            start = Offset(begin, size.height * LineBottomY),
            end = Offset(size.width * LineShortEnd, size.height * LineBottomY),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
    }
}

@Composable
private fun TicketGrid(entries: List<WatchEntry>, onOpen: (WatchEntry) -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        entries.chunked(Columns).forEach { row ->
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
            ) {
                row.forEach { entry ->
                    TicketCard(entry = entry, onOpen = onOpen, modifier = Modifier.weight(1f))
                }
                // 单数行的空位用同权重占位，保持左列宽度与双列一致。
                if (row.size < Columns) Spacer(Modifier.weight(1f))
            }
        }
    }
}

@Composable
private fun TicketCard(entry: WatchEntry, onOpen: (WatchEntry) -> Unit, modifier: Modifier) {
    Column(
        modifier = modifier
            .clip(CardShape)
            .background(DesignTokens.canvasFill)
            .border(HairlineWidth, DesignTokens.hairline, CardShape)
            .clickable(role = Role.Button) { onOpen(entry) }
            .padding(TicketPad),
    ) {
        EntryCover(entry = entry, aspectRatio = WideRatio)
        Spacer(Modifier.height(TicketTextGap))
        Text(
            text = entry.title,
            fontSize = TicketTitleSize.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
        Text(
            text = entry.subtitle,
            fontSize = TicketSubtitleSize.sp,
            color = DesignTokens.textSub,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

private const val SectionTitle = "待看"
private const val EmptyTitle = "还没有待看的影片"
private const val EmptyMessage = "在详情页加入待看，票根会出现在这里"
private const val Columns = 2
private const val EmptyTitleSize = 16f
private const val EmptyMessageSize = 13f
private const val TicketTitleSize = 15f
private const val TicketSubtitleSize = 13f
private const val WideRatio = 16f / 9f

// 内线比例按票根图标的视觉重心取：左右缩进 22%，两条线分别落在 42% / 64% 高度。
private const val LineBegin = 0.22f
private const val LineFullEnd = 0.78f
private const val LineShortEnd = 0.6f
private const val LineTopY = 0.42f
private const val LineBottomY = 0.64f

private val CardShape = RoundedCornerShape(16.dp)
private val HairlineWidth = 1.dp
private val EmptyCardPad = 22.dp
private val GlyphGap = 12.dp
private val EmptyTitleGap = 4.dp
private val GlyphWidth = 32.dp
private val GlyphHeight = 24.dp
private val GlyphStroke = 1.5f.dp
private val GlyphCorner = 4.dp
private val TicketPad = 10.dp
private val TicketTextGap = 6.dp

@Preview(name = "待看 · 空态", widthDp = 390, showBackground = true)
@Composable
private fun PreviewWatchlistEmpty() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp)) {
        WatchlistSection(entries = emptyList(), onOpen = {}, onAdd = {})
    }
}

@Preview(name = "待看 · 两列票根", widthDp = 390, showBackground = true)
@Composable
private fun PreviewWatchlistFilled() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp)) {
        WatchlistSection(
            entries = listOf(
                WatchEntry("天国王朝", "2005 · 电影", posterPath = null, progressPercent = null),
                WatchEntry("漫长的季节", "2023 · 剧集 · 12 集", posterPath = null, progressPercent = null),
                WatchEntry("沙丘 2", "2024 · 电影", posterPath = null, progressPercent = null),
            ),
            onOpen = {},
            onAdd = {},
        )
    }
}