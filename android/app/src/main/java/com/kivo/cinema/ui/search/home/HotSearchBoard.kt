package com.kivo.cinema.ui.search.home

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.search.SearchGlyph
import com.kivo.cinema.ui.search.SearchIcon
import com.kivo.cinema.ui.search.SearchTokens

/**
 * ① 热搜榜：一整张白卡 + 行（排名 / 40×60 缩略图 / 片名+元信息 / NEW）。
 * 数据源 = TMDB trending/day；没有名次升降（无真实来源，family-tree D3）。
 */
@Composable
internal fun HotSearchBoard(
    items: List<SearchEntry>,
    loading: Boolean,
    error: String?,
    onRetry: () -> Unit,
    onSelect: (SearchEntry) -> Unit,
) {
    Column(Modifier.padding(horizontal = SearchTokens.pad)) {
        Row(
            Modifier.fillMaxWidth().padding(bottom = 10.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text("热搜榜", fontSize = 20.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.clip(RoundedCornerShape(8.dp)).clickable(onClick = onRetry)
                    .padding(horizontal = 8.dp, vertical = 6.dp),
            ) {
                Text("今日", fontSize = 13.sp, color = DesignTokens.textSub)
                SearchGlyph(SearchIcon.ChevronDown, DesignTokens.textSub, Modifier.size(13.dp))
            }
        }
        Column(
            Modifier.fillMaxWidth().clip(RoundedCornerShape(16.dp))
                .background(SearchTokens.boardFill)
                .border(1.dp, DesignTokens.hairline, RoundedCornerShape(16.dp))
                .padding(8.dp),
        ) {
            when {
                loading && items.isEmpty() -> BoardHint("正在加载今日热搜…")
                error != null && items.isEmpty() -> BoardHint("$error，点这里重试", onRetry)
                items.isEmpty() -> BoardHint("今日热搜暂时为空")
                else -> items.forEachIndexed { index, entry ->
                    HotRow(index + 1, entry, onSelect)
                }
            }
        }
    }
}

@Composable
private fun BoardHint(text: String, onClick: (() -> Unit)? = null) {
    Text(
        text, fontSize = 13.sp, color = DesignTokens.textSub,
        modifier = Modifier.fillMaxWidth().padding(14.dp)
            .then(if (onClick != null) Modifier.clickable(onClick = onClick) else Modifier),
    )
}

@Composable
private fun HotRow(rank: Int, entry: SearchEntry, onSelect: (SearchEntry) -> Unit) {
    Row(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(10.dp))
            .clickable { onSelect(entry) }
            .padding(start = 4.dp, end = 6.dp, top = 6.dp, bottom = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            "$rank", fontSize = 17.sp, fontWeight = FontWeight.ExtraBold,
            color = if (rank <= 3) DesignTokens.textMain else SearchTokens.textFaint,
            modifier = Modifier.width(26.dp), textAlign = androidx.compose.ui.text.style.TextAlign.Center,
            maxLines = 1,
        )
        Spacer(Modifier.size(12.dp))
        TmdbImage(
            entry.posterUrl.orEmpty(), entry.title,
            Modifier.width(SearchTokens.hotThumbWidth).height(SearchTokens.hotThumbHeight)
                .clip(RoundedCornerShape(8.dp)),
        )
        Spacer(Modifier.size(12.dp))
        Column(Modifier.weight(1f)) {
            Text(
                entry.title, fontSize = 15.sp, fontWeight = FontWeight.Medium,
                color = DesignTokens.textMain, maxLines = 1, overflow = TextOverflow.Ellipsis,
            )
            Row(Modifier.padding(top = 3.dp), verticalAlignment = Alignment.CenterVertically) {
                Text(
                    listOf(entry.kindLabel, entry.genres.firstOrNull().orEmpty())
                        .filter { it.isNotEmpty() }.joinToString(" · ") + " · ",
                    fontSize = 12.sp, color = DesignTokens.textSub,
                )
                Text("★", color = V6Metrics.brass, fontSize = 11.sp)
                Text(" %.1f".format(entry.rating), fontSize = 12.sp, color = DesignTokens.textSub)
            }
        }
        if (entry.isNew) Text(
            "NEW", fontSize = 10.sp, fontWeight = FontWeight.Bold, letterSpacing = .3.sp,
            color = DesignTokens.textMain,
            modifier = Modifier.border(1.dp, DesignTokens.textMain.copy(alpha = .28f), RoundedCornerShape(5.dp))
                .padding(horizontal = 5.dp),
        )
    }
}