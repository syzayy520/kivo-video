package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 本周趋势：宽卡 161×90.6 (16:9) 横滑 2.2 列露头（E8），标签「2026 · 恐怖 / 科幻」。✅ 等值搬迁。 */
@Composable
internal fun WideSection(
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader("本周趋势", fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            items(list) { m ->
                Column(
                    Modifier
                        .width(161.dp)
                        .clickable { onOpenTmdbItem(m) },
                ) {
                    Box(
                        Modifier
                            .size(161.dp, 91.dp)
                            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                    ) { TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize()) }
                    Spacer(Modifier.height(6.dp))
                    Text(m.title, fontSize = 15.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Text(m.tagLine(), fontSize = 13.sp, color = chromeSubFor(fg),
                        maxLines = 1, overflow = TextOverflow.Ellipsis)
                }
            }
        }
    }
}