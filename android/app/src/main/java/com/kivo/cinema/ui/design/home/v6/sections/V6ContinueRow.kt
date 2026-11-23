package com.kivo.cinema.ui.design.home.v6.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.ui.design.home.v6.SourceStrip
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.V6SectionHeader
import com.kivo.cinema.ui.design.home.v6.qualitySegments

/**
 * 继续观看（紧凑卡）：16:9 228×128，一屏露出 1.6 张；图内只放片源签条与「进度条 + 剩余时间」，
 * 片名与季集移到卡下（对标参考作的紧凑续播卡，图更干净、一屏信息更多）。
 * 画质段只对本地 / SMB 解析文件名；Emby 条目没有文件名就只写来源。
 */
@Composable
internal fun V6ContinueRow(
    entries: List<HomeEntry>,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpen: (HomeEntry) -> Unit,
    onMore: (() -> Unit)?,
) {
    if (entries.isEmpty()) return
    Column {
        V6SectionHeader("继续观看", fg, sub, onMore = onMore)
        Spacer(Modifier.height(V6Metrics.headerGap))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            contentPadding = PaddingValues(horizontal = V6Metrics.screenPad),
        ) {
            items(entries) { entry -> ContinueCard(entry, fg, sub, posterOf, onOpen) }
        }
    }
}

@Composable
private fun ContinueCard(
    entry: HomeEntry,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpen: (HomeEntry) -> Unit,
) {
    val shape = RoundedCornerShape(V6Metrics.rCard)
    Column(Modifier.width(V6Metrics.continueW).clickable { onOpen(entry) }) {
        Box(Modifier.size(V6Metrics.continueW, V6Metrics.continueH).clip(shape).background(Color(0xFF2A2C32))) {
            Box(Modifier.fillMaxSize()) { posterOf(entry)?.invoke() }
            Box(Modifier.fillMaxSize().background(
                Brush.verticalGradient(0f to Color.Transparent, .55f to Color.Transparent, 1f to Color(0xB30A0A0E))))
            val quality = if (entry.kind == HomeEntry.Kind.Emby) emptyList()
                else qualitySegments(entry.key.substringAfterLast('/'))
            SourceStrip(quality, entry.sourceLabel, Modifier.padding(10.dp))
            Row(
                Modifier.align(Alignment.BottomStart).fillMaxWidth().padding(horizontal = 12.dp, vertical = 10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                if (entry.progress > 0f) {
                    Box(Modifier.weight(1f).height(3.dp).clip(RoundedCornerShape(2.dp)).background(Color.White.copy(alpha = .3f))) {
                        Box(Modifier.fillMaxWidth(entry.progress).height(3.dp).background(V6Metrics.brass))
                    }
                    Spacer(Modifier.width(8.dp))
                }
                Text(timeOf(entry), color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Medium, maxLines = 1)
            }
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnDark, shape))
        }
        Spacer(Modifier.height(8.dp))
        Text(entry.title, color = fg, fontSize = V6Metrics.fsWideTitle, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        // 来源已在签条里，这里不重复；没有季集时写年份。
        Text(episodeOf(entry) ?: entry.year?.takeIf { it > 0 }?.toString().orEmpty(), color = sub, fontSize = V6Metrics.fsMeta, maxLines = 1,
            overflow = TextOverflow.Ellipsis)
    }
}

/** 「S01E03」→「第 1 季 · 第 3 集」；没有季集信息返回 null。 */
private fun episodeOf(entry: HomeEntry): String? =
    entry.episodeLabel?.let { Regex("""S(\d+)E(\d+)""", RegexOption.IGNORE_CASE).find(it) }
        ?.let { "第 ${it.groupValues[1].toInt()} 季 · 第 ${it.groupValues[2].toInt()} 集" }

/** 「剩 42 分钟」；时长未知时退回「看到 12:34」。 */
private fun timeOf(entry: HomeEntry): String =
    if (entry.durationMs > 0) "剩 " + remainingText(entry.durationMs - entry.positionMs)
    else "看到 " + clock(entry.positionMs)

private fun remainingText(ms: Long): String {
    val min = (ms.coerceAtLeast(0) / 60_000).toInt().coerceAtLeast(1)
    return if (min < 60) "$min 分钟" else "${min / 60} 小时 ${"%02d".format(min % 60)} 分"
}

private fun clock(ms: Long): String {
    val s = (ms / 1000).toInt()
    return if (s >= 3600) "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60) else "%d:%02d".format(s / 60, s % 60)
}
