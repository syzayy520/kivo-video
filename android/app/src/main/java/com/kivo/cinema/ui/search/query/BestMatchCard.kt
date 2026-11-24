package com.kivo.cinema.ui.search.query

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.history.TintedArtwork
import com.kivo.cinema.ui.history.glassSurface
import com.kivo.cinema.ui.search.SearchGlyph
import com.kivo.cinema.ui.search.SearchIcon
import com.kivo.cinema.ui.search.SearchTokens

/**
 * ② 最佳匹配大卡：剧照底 + 取色模糊渐变（复用 TintedArtwork，veil 72% / blur 24）。
 * 「播放」钮只在真实可播时出现（打开既有详情页，由详情页完成起播）；未入库不画假播放。
 */
@Composable
internal fun BestMatchCard(entry: SearchEntry, onOpen: (SearchEntry) -> Unit) {
    val shape = RoundedCornerShape(SearchTokens.bestRadius)
    Box(Modifier.padding(start = SearchTokens.pad, top = 14.dp, end = SearchTokens.pad)) {
        TintedArtwork(
            artwork = entry.backdropUrl?.let { url ->
                { TmdbImage(url, entry.title, Modifier.fillMaxSize()) }
            },
            fallbackLetter = entry.title.take(1),
            veilFraction = .72f,
            blurDp = 24.dp,
            shape = shape,
            modifier = Modifier.fillMaxWidth().height(SearchTokens.bestHeight)
                .clip(shape).clickable { onOpen(entry) },
        ) {
            Column(
                Modifier.align(Alignment.BottomStart).fillMaxWidth()
                    .padding(start = 16.dp, end = 16.dp, bottom = 14.dp),
            ) {
                Text(
                    "最佳匹配 · ${entry.kindLabel}", fontSize = 11.sp, letterSpacing = 1.2.sp,
                    color = DesignTokens.canvasFill.copy(alpha = .82f), fontWeight = FontWeight.SemiBold,
                )
                Text(
                    entry.title, fontSize = 22.sp, fontWeight = FontWeight.Bold,
                    color = DesignTokens.canvasFill, maxLines = 1, overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.padding(top = 3.dp, bottom = 2.dp),
                )
                Text(
                    bestMeta(entry), fontSize = 12.sp, color = DesignTokens.canvasFill.copy(alpha = .8f),
                    maxLines = 1, overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.padding(end = 104.dp),
                )
            }
            if (entry.playable) Row(
                Modifier.align(Alignment.BottomEnd).padding(10.dp)
                    .height(34.dp)
                    .glassSurface(RoundedCornerShape(17.dp), onImage = true)
                    .clip(RoundedCornerShape(17.dp))
                    .clickable { onOpen(entry) }
                    .padding(horizontal = 16.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                SearchGlyph(SearchIcon.Play, DesignTokens.canvasFill, Modifier.size(12.dp))
                Spacer(Modifier.size(6.dp))
                Text("播放", fontSize = 14.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.canvasFill)
            }
        }
    }
}

private fun bestMeta(entry: SearchEntry): String {
    val parts = mutableListOf<String>()
    if (entry.year.isNotEmpty()) parts += entry.year
    if (entry.genres.isNotEmpty()) parts += entry.genres.take(2).joinToString(" / ")
    if (entry.rating > 0f) parts += "★ %.1f".format(entry.rating)
    return parts.joinToString(" · ")
}