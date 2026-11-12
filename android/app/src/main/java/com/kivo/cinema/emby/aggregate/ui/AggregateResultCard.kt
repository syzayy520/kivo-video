package com.kivo.cinema.emby.aggregate.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.aggregate.AggregateSearchModel

/**
 * 256 聚合结果卡：海报 + 左上类型徽标 + 右下评分 + 标题 + 日期/共N季。
 * 单一职责：把一条合并结果画成截图里的卡片；徽标/评分/日期全部来自条目真实字段。
 */
@Composable
fun AggregateResultCard(result: AggregateSearchModel.Result, onClick: () -> Unit) {
    val item = result.item
    val badge = when {
        item.genres.any { it == "纪录" || it == "纪录片" } -> "纪录片"
        item.series -> "电视剧"
        else -> "电影"
    }
    val sub = when {
        item.series && item.childCount != null -> "共${item.childCount}季"
        item.premiereDate.isNotBlank() -> item.premiereDate
        item.year != null -> "${item.year}"
        else -> ""
    }
    Column(Modifier.clip(RoundedCornerShape(12.dp)).clickable(
        interactionSource = remember { MutableInteractionSource() }, indication = null, onClick = onClick)) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f)) {
            AggregatePoster(result.source, item.posterUrl, Modifier.fillMaxSize())
            Text(badge, fontSize = 10.sp, color = Color.White, fontWeight = FontWeight.Medium,
                modifier = Modifier.align(Alignment.TopStart).padding(6.dp)
                    .clip(RoundedCornerShape(6.dp)).background(Color(0x99000000))
                    .padding(horizontal = 6.dp, vertical = 2.dp))
            item.rating?.let {
                Text(String.format("%.1f", it), fontSize = 11.sp, color = Color(0xFFFFD34D),
                    fontWeight = FontWeight.Bold,
                    modifier = Modifier.align(Alignment.BottomEnd).padding(6.dp)
                        .clip(RoundedCornerShape(6.dp)).background(Color(0x99000000))
                        .padding(horizontal = 6.dp, vertical = 2.dp))
            }
        }
        Spacer(Modifier.height(6.dp))
        Text(item.name, fontSize = 13.sp, fontWeight = FontWeight.Medium, maxLines = 1,
            overflow = TextOverflow.Ellipsis, color = MaterialTheme.colorScheme.onSurface)
        if (sub.isNotBlank()) Text(sub, fontSize = 11.sp, maxLines = 1,
            color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.padding(top = 2.dp))
    }
}