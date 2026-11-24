package com.kivo.cinema.ui.search.query

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.search.SearchTokens

/** ② 结果列表：52×78 缩略图 + 关键词高亮片名 + 元信息 + 来源签条；行间细分隔线内缩 64。 */
@Composable
internal fun SearchResultList(
    entries: List<SearchEntry>,
    query: String,
    onOpen: (SearchEntry) -> Unit,
) {
    Column(Modifier.padding(horizontal = SearchTokens.pad)) {
        entries.forEachIndexed { index, entry ->
            if (index > 0) HorizontalDivider(
                Modifier.padding(start = 64.dp), thickness = 1.dp, color = DesignTokens.hairline,
            )
            ResultRow(entry, query, onOpen)
        }
    }
}

@Composable
private fun ResultRow(entry: SearchEntry, query: String, onOpen: (SearchEntry) -> Unit) {
    Row(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp))
            .clickable { onOpen(entry) }.padding(vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        TmdbImage(
            entry.posterUrl.orEmpty(), entry.title,
            Modifier.width(SearchTokens.resultThumbWidth).height(SearchTokens.resultThumbHeight)
                .clip(RoundedCornerShape(9.dp)),
        )
        Spacer(Modifier.size(12.dp))
        Column(Modifier.weight(1f)) {
            Text(
                highlightTitle(entry.title, query), fontSize = 15.sp, fontWeight = FontWeight.Medium,
                maxLines = 1, overflow = TextOverflow.Ellipsis,
            )
            Row(Modifier.padding(top = 3.dp), verticalAlignment = Alignment.CenterVertically) {
                Text(metaText(entry), fontSize = 12.sp, color = DesignTokens.textSub,
                    maxLines = 1, overflow = TextOverflow.Ellipsis)
                if (entry.rating > 0f) {
                    Text(" · ", fontSize = 12.sp, color = DesignTokens.textSub)
                    Text("★", color = V6Metrics.brass, fontSize = 11.sp)
                    Text(" %.1f".format(entry.rating), fontSize = 12.sp, color = DesignTokens.textSub)
                }
            }
            Row(Modifier.padding(top = 5.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(6.dp).clip(CircleShape)
                        .background(if (entry.playable) SearchTokens.playableDot else SearchTokens.idleDot),
                )
                Spacer(Modifier.size(5.dp))
                Text(
                    sourceText(entry), fontSize = 11.sp, fontWeight = FontWeight.Medium,
                    color = if (entry.playable) SearchTokens.playableText else DesignTokens.textSub,
                )
            }
        }
    }
}

/** 关键词命中片段加粗 + 黄铜浅底（稿面 mark 的黄铜下划线以浅底近似，不整行高亮）。 */
private fun highlightTitle(title: String, query: String) = buildAnnotatedString {
    val q = query.trim()
    val start = if (q.isEmpty()) -1 else title.indexOf(q, ignoreCase = true)
    if (start < 0) {
        withStyle(SpanStyle(color = DesignTokens.textMain)) { append(title) }
        return@buildAnnotatedString
    }
    withStyle(SpanStyle(color = DesignTokens.textMain)) { append(title.substring(0, start)) }
    withStyle(
        SpanStyle(
            color = DesignTokens.textMain, fontWeight = FontWeight.Bold,
            background = V6Metrics.brass.copy(alpha = .30f),
        ),
    ) { append(title.substring(start, start + q.length)) }
    withStyle(SpanStyle(color = DesignTokens.textMain)) { append(title.substring(start + q.length)) }
}

private fun metaText(entry: SearchEntry): String {
    val parts = mutableListOf(entry.kindLabel)
    if (entry.year.isNotEmpty()) parts += entry.year
    entry.genres.firstOrNull()?.let { parts += it }
    return parts.joinToString(" · ")
}

private fun sourceText(entry: SearchEntry): String =
    if (entry.quality != null) "${entry.sourceLabel} · ${entry.quality}" else entry.sourceLabel