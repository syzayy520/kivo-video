package com.kivo.cinema.ui.search.home

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.search.SearchTokens

/** ① 最近搜索：标题 + 清除 + 横滑 chips；无记录时整段不渲染（真实空态）。 */
@Composable
internal fun RecentSearchChips(
    terms: List<String>,
    onSelect: (String) -> Unit,
    onClear: () -> Unit,
) {
    if (terms.isEmpty()) return
    Column {
        Row(
            Modifier.fillMaxWidth().padding(horizontal = SearchTokens.pad),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text("最近搜索", fontSize = 20.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            Text(
                "清除", fontSize = 13.sp, color = DesignTokens.textSub,
                modifier = Modifier.clip(RoundedCornerShape(8.dp)).clickable(onClick = onClear)
                    .padding(horizontal = 8.dp, vertical = 8.dp),
            )
        }
        LazyRow(
            contentPadding = PaddingValues(horizontal = SearchTokens.pad),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            modifier = Modifier.padding(top = 10.dp),
        ) {
            items(terms, key = { it }) { term ->
                Box(
                    Modifier.height(SearchTokens.chipHeight)
                        .clip(RoundedCornerShape(17.dp))
                        .background(SearchTokens.chipFill)
                        .border(1.dp, DesignTokens.hairline, RoundedCornerShape(17.dp))
                        .clickable { onSelect(term) }
                        .padding(horizontal = 14.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(term, fontSize = 14.sp, color = DesignTokens.textMain, maxLines = 1)
                }
            }
        }
    }
}