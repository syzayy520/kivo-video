package com.kivo.cinema.ui.search.home

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchCategory
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.history.TintedArtwork
import com.kivo.cinema.ui.search.SearchTokens

/** ① 分类：3 列 × 2 行，六张卡 = [SearchCategory.cards] 一份数据，一屏全见（定稿不横滑）。 */
@Composable
internal fun CategoryCardGrid(
    artworks: Map<SearchCategory, String>,
    onSelect: (SearchCategory) -> Unit,
) {
    Column(
        Modifier.padding(horizontal = SearchTokens.pad),
        verticalArrangement = Arrangement.spacedBy(SearchTokens.categoryGap),
    ) {
        SearchCategory.cards.chunked(3).forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(SearchTokens.categoryGap)) {
                row.forEach { category ->
                    CategoryCard(category, artworks[category], Modifier.weight(1f)) { onSelect(category) }
                }
            }
        }
    }
}

@Composable
private fun CategoryCard(
    category: SearchCategory,
    artworkUrl: String?,
    modifier: Modifier,
    onClick: () -> Unit,
) {
    TintedArtwork(
        artwork = artworkUrl?.let { url ->
            { TmdbImage(url, category.label, Modifier.fillMaxSize()) }
        },
        fallbackLetter = category.label.take(1),
        veilFraction = .8f,
        blurDp = 12.dp,
        shape = RoundedCornerShape(SearchTokens.categoryRadius),
        modifier = modifier.height(SearchTokens.categoryHeight)
            .clip(RoundedCornerShape(SearchTokens.categoryRadius))
            .clickable(onClick = onClick),
    ) {
        Column(
            Modifier.align(Alignment.BottomStart).fillMaxWidth()
                .padding(start = 10.dp, end = 10.dp, bottom = 9.dp),
        ) {
            Text(
                category.label, color = DesignTokens.canvasFill,
                fontSize = 16.sp, fontWeight = FontWeight.SemiBold,
            )
            Text(
                category.english, color = DesignTokens.canvasFill.copy(alpha = .82f),
                fontSize = 8.5.sp, fontWeight = FontWeight.Medium, letterSpacing = 1.6.sp,
            )
        }
    }
}