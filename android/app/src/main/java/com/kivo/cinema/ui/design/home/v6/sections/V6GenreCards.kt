package com.kivo.cinema.ui.design.home.v6.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
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
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbGenre
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.V6SectionHeader

/**
 * 分类浏览：整张剧照打底的图卡（220×124），左下大号中文名 + 英文小字（对标 Apple TV「Browse by Genre」）。
 * 剧照由 TmdbGenres.fillDistinctBackdrops 保证互不重复；没有剧照的类型用按 id 稳定的深色渐变，不留空框。
 */
@Composable
internal fun V6GenreCards(
    genres: List<TmdbGenre>,
    fg: Color,
    sub: Color,
    onOpen: (TmdbGenre) -> Unit,
) {
    if (genres.isEmpty()) return
    Column {
        V6SectionHeader("分类浏览", fg, sub)
        Spacer(Modifier.height(V6Metrics.headerGap))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            contentPadding = PaddingValues(horizontal = V6Metrics.screenPad),
        ) {
            items(genres) { g -> GenreCard(g, onOpen) }
        }
    }
}

@Composable
private fun GenreCard(g: TmdbGenre, onOpen: (TmdbGenre) -> Unit) {
    val shape = RoundedCornerShape(V6Metrics.rCard)
    val hue = ((g.id * 47) % 360).toFloat()
    Box(
        Modifier.size(V6Metrics.genreW, V6Metrics.genreH).clip(shape)
            .background(Brush.linearGradient(listOf(Color.hsl(hue, .32f, .36f), Color.hsl((hue + 30) % 360, .36f, .2f))))
            .clickable { onOpen(g) },
    ) {
        g.backdropUrl?.let { TmdbImage(it, g.name, Modifier.fillMaxSize(), bg = Color.Transparent) }
        Box(Modifier.fillMaxSize().background(Brush.horizontalGradient(
            0f to Color.Black.copy(alpha = .62f), .55f to Color.Black.copy(alpha = .18f), 1f to Color.Transparent)))
        Box(Modifier.fillMaxSize().background(Brush.verticalGradient(
            .45f to Color.Transparent, 1f to Color.Black.copy(alpha = .45f))))
        Column(Modifier.align(Alignment.BottomStart).padding(start = 14.dp, bottom = 12.dp)) {
            Text(g.name, color = Color.White, fontSize = 22.sp, fontWeight = FontWeight.Bold)
            ENGLISH[g.id]?.let {
                Text(it, color = Color.White.copy(alpha = .72f), fontSize = 9.sp,
                    fontWeight = FontWeight.SemiBold, letterSpacing = 2.sp)
            }
        }
        Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnDark, shape))
    }
}

/** TMDB 电影类型 id → 英文副标（固定词表，与 /genre/movie/list 的 19 个 id 一一对应）。 */
private val ENGLISH = mapOf(
    28 to "ACTION", 12 to "ADVENTURE", 16 to "ANIMATION", 35 to "COMEDY", 80 to "CRIME",
    99 to "DOCUMENTARY", 18 to "DRAMA", 10751 to "FAMILY", 14 to "FANTASY", 36 to "HISTORY",
    27 to "HORROR", 10402 to "MUSIC", 9648 to "MYSTERY", 10749 to "ROMANCE", 878 to "SCI-FI",
    10770 to "TV MOVIE", 53 to "THRILLER", 10752 to "WAR", 37 to "WESTERN",
)
