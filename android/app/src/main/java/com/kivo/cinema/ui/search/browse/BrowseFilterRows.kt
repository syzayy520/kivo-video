package com.kivo.cinema.ui.search.browse

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.BrowseFilter
import com.kivo.cinema.search.SEARCH_GENRES
import com.kivo.cinema.search.SEARCH_REGIONS
import com.kivo.cinema.search.SEARCH_YEARS
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.search.SearchTokens

/** ③ 三行筛选：类型/地区/年份，同一份行组件 × 三份数据；选中 = 黑底白字胶囊。 */
@Composable
internal fun BrowseFilterRows(filter: BrowseFilter, onChange: (BrowseFilter) -> Unit) {
    Column {
        FilterRow("类型", SEARCH_GENRES.map { it.second }, filter.genreLabel) { label ->
            onChange(filter.copy(genreId = SEARCH_GENRES.first { it.second == label }.first))
        }
        FilterRow("地区", SEARCH_REGIONS.map { it.second }, filter.regionLabel) { label ->
            onChange(filter.copy(region = SEARCH_REGIONS.first { it.second == label }.first))
        }
        FilterRow("年份", SEARCH_YEARS, filter.year) { label ->
            onChange(filter.copy(year = label))
        }
    }
}

@Composable
private fun FilterRow(
    label: String,
    options: List<String>,
    selected: String,
    onSelect: (String) -> Unit,
) {
    Row(Modifier.height(38.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(
            label, fontSize = 12.sp, color = DesignTokens.textSub,
            modifier = Modifier.width(36.dp).padding(start = SearchTokens.pad),
        )
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(2.dp),
            contentPadding = PaddingValues(end = SearchTokens.pad),
        ) {
            items(options, key = { it }) { option ->
                val on = option == selected
                Text(
                    option, fontSize = 13.sp,
                    color = if (on) DesignTokens.canvasFill else SearchTokens.textFilter,
                    fontWeight = if (on) FontWeight.SemiBold else FontWeight.Normal,
                    modifier = Modifier.clip(RoundedCornerShape(15.dp))
                        .background(if (on) DesignTokens.textMain else androidx.compose.ui.graphics.Color.Transparent)
                        .clickable { onSelect(option) }
                        .padding(horizontal = 12.dp, vertical = 6.dp),
                )
            }
        }
    }
}