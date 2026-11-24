package com.kivo.cinema.ui.search.browse

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.BrowseFilter
import com.kivo.cinema.search.SEARCH_SORTS
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.history.glassSurface
import com.kivo.cinema.ui.search.SearchGlyph
import com.kivo.cinema.ui.search.SearchIcon
import com.kivo.cinema.ui.search.SearchTokens

/** ③ 工具行：左「共 N 部」；右排序小下拉 + 只看可播开关。 */
@Composable
internal fun BrowseToolRow(
    countText: String,
    filter: BrowseFilter,
    onChange: (BrowseFilter) -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().padding(horizontal = SearchTokens.pad, vertical = 10.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("共 ", fontSize = 12.sp, color = DesignTokens.textSub)
            Text("$countText", fontSize = 12.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
            Text(" 部", fontSize = 12.sp, color = DesignTokens.textSub)
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            SortDropdown(filter, onChange)
            Spacer(Modifier.size(10.dp))
            Text("只看可播", fontSize = 12.sp, color = SearchTokens.textFilter)
            Switch(
                checked = filter.onlyPlayable,
                onCheckedChange = { onChange(filter.copy(onlyPlayable = it)) },
                modifier = Modifier.scale(.8f),
                colors = SwitchDefaults.colors(
                    checkedTrackColor = SearchTokens.playableDot,
                    checkedThumbColor = DesignTokens.canvasFill,
                ),
            )
        }
    }
}

@Composable
private fun SortDropdown(filter: BrowseFilter, onChange: (BrowseFilter) -> Unit) {
    var expanded by remember { mutableStateOf(false) }
    Box {
        Row(
            Modifier.height(30.dp)
                .glassSurface(RoundedCornerShape(15.dp))
                .clip(RoundedCornerShape(15.dp))
                .clickable { expanded = true }
                .padding(start = 12.dp, end = 10.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(filter.sortLabel, fontSize = 12.sp, fontWeight = FontWeight.Medium, color = DesignTokens.textMain)
            SearchGlyph(SearchIcon.ChevronDown, DesignTokens.textSub, Modifier.size(13.dp))
        }
        DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
            SEARCH_SORTS.forEachIndexed { index, pair ->
                DropdownMenuItem(
                    text = { Text(pair.second, fontSize = 14.sp) },
                    onClick = {
                        expanded = false
                        onChange(filter.copy(sortIndex = index))
                    },
                )
            }
        }
    }
}

/** ④ 吸顶摘要：整条可点展开（滚回筛选区），含全部非默认条件与排序（D4）。 */
@Composable
internal fun StickySummaryPill(summary: String, countText: String, onExpand: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(horizontal = SearchTokens.pad, vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Row(
            Modifier.height(34.dp)
                .glassSurface(RoundedCornerShape(17.dp))
                .clip(RoundedCornerShape(17.dp))
                .clickable(onClick = onExpand)
                .padding(start = 12.dp, end = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            SearchGlyph(SearchIcon.Filter, DesignTokens.textMain, Modifier.size(15.dp))
            Spacer(Modifier.size(8.dp))
            Text(summary, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
            Spacer(Modifier.size(8.dp))
            SearchGlyph(SearchIcon.ChevronDown, DesignTokens.textSub, Modifier.size(12.dp))
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("共 ", fontSize = 12.sp, color = DesignTokens.textSub)
            Text("$countText", fontSize = 12.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
            Text(" 部", fontSize = 12.sp, color = DesignTokens.textSub)
        }
    }
}