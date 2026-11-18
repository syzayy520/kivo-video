package com.kivo.cinema.sources.ui.cards

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.overview.SourceSummaries.Summary
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 全宽媒体服务器卡：品牌图标 + 名称 + 在线状态 + 计数 + 最近海报条 + 管理入口。
 * 单一职责：Emby 服务器的概览呈现；点击进库、长按管理语义与 SourceCard 一致。
 * 海报失败在 SourcePosterStrip 内降级为空，不影响本卡其余信息。
 */
@OptIn(androidx.compose.foundation.ExperimentalFoundationApi::class)
@Composable
internal fun ServerCard(record: SourceRecord, summary: Summary?,
    onOpen: () -> Unit, onManage: () -> Unit,
) {
    val counts = summary?.counts
    val subtitle = when {
        counts != null -> "电影:${counts.movies}  剧集:${counts.series}"
        summary?.reason != null -> summary.reason!!
        else -> "正在读取数量…"
    }
    val online = summary?.online
    Surface(
        modifier = Modifier.fillMaxWidth().combinedClickable(
            onClick = onOpen, onLongClick = onManage, onLongClickLabel = "管理${record.name}",
        ),
        shape = RoundedCornerShape(20.dp),
        border = BorderStroke(0.5.dp, Color(0xFFDFDFE6)),
        color = Color.White.copy(alpha = 0.9f),
    ) {
        Column(Modifier.fillMaxWidth().padding(14.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                SourceIcon(SourceIconKind.EMBY, Modifier.size(30.dp))
                Spacer(Modifier.width(10.dp))
                Column(Modifier.weight(1f)) {
                    Text(record.name, fontSize = 14.sp, fontWeight = FontWeight.Medium,
                        color = DesignTokens.textMain, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        if (summary?.reason == null) {
                            StatusDot(online)
                            Spacer(Modifier.width(5.dp))
                        }
                        Text(subtitle, fontSize = 11.sp, color = DesignTokens.textSub,
                            maxLines = 1, overflow = TextOverflow.Ellipsis)
                    }
                }
                IconButton(onClick = onManage, modifier = Modifier.size(48.dp)
                    .semantics { contentDescription = "管理${record.name}" }) {
                    Text("⋯", fontSize = 24.sp, color = DesignTokens.textSub)
                }
            }
            val posters = summary?.posters.orEmpty()
            if (posters.isNotEmpty()) {
                Spacer(Modifier.height(10.dp))
                SourcePosterStrip(record.address, summary?.imageToken, posters)
            }
        }
    }
}

@Composable
private fun StatusDot(online: Boolean?) {
    val color = when (online) {
        true -> DesignTokens.accentGreen
        false -> DesignTokens.dangerRed
        null -> DesignTokens.textSub
    }
    val desc = when (online) {
        true -> "服务器在线，最近检测可访问"
        false -> "服务器连接失败或服务异常"
        null -> "服务器状态待检测"
    }
    androidx.compose.foundation.layout.Box(
        Modifier.size(7.dp).background(color, RoundedCornerShape(50))
            .semantics { contentDescription = desc },
    )
}
