package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.res.painterResource
import com.kivo.cinema.R
import com.kivo.cinema.sources.SourceRecord

@OptIn(androidx.compose.foundation.ExperimentalFoundationApi::class)
@Composable
fun SourceCard(record: SourceRecord, onOpen: () -> Unit, onManage: () -> Unit,
    summary: com.kivo.cinema.sources.overview.SourceSummaries.Summary? = null) {
    Surface(modifier = Modifier.combinedClickable(onClick = onOpen,
        onLongClick = onManage, onLongClickLabel = "管理${record.name}"), shape = RoundedCornerShape(20.dp),
        border = BorderStroke(.5.dp, Color(0xFFDFDFE6)), color = Color.White.copy(alpha = .9f)) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Image(painterResource(R.drawable.emby_logo), contentDescription = "Emby",
                    modifier = Modifier.size(30.dp))
                Spacer(Modifier.weight(1f))
                Box(Modifier.size(6.dp).background(when (summary?.online) {
                    true -> Color(0xFF8EEA22)
                    false -> Color(0xFFE65361)
                    null -> Color(0xFFB8BBC4)
                }, CircleShape).semantics {
                    contentDescription = when (summary?.online) {
                        true -> "服务器在线，最近检测可访问"
                        false -> "服务器连接失败或服务异常"
                        null -> "服务器状态待检测"
                    }
                })
                IconButton(onClick = onManage, modifier = Modifier.size(48.dp)
                    .semantics { contentDescription = "管理${record.name}" }) {
                    Text("⋯", fontSize = 24.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }
            Text(record.name, fontWeight = FontWeight.Normal, fontSize = 14.sp, lineHeight = 18.sp,
                maxLines = 1, overflow = TextOverflow.Ellipsis)
            val counts = summary?.counts
            Text(when {
                counts != null -> "电影:${counts.movies}  剧集:${counts.series}"
                summary?.reason != null -> summary.reason
                else -> "正在读取数量…"
            }, fontSize = 11.sp, lineHeight = 15.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
    }
}
