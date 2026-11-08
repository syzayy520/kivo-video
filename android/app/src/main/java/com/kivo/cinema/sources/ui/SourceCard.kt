package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
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
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.sources.SourceRecord

@Composable
fun SourceCard(record: SourceRecord, onOpen: () -> Unit, onManage: () -> Unit) {
    Surface(onClick = onOpen, shape = RoundedCornerShape(DesignTokens.radiusCard.dp), color = Color.White.copy(alpha = .84f)) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Surface(shape = RoundedCornerShape(7.dp), color = Color(0xFFEAF2ED)) {
                    Text("EMBY", Modifier.padding(horizontal = 10.dp, vertical = 6.dp),
                        style = MaterialTheme.typography.labelMedium, color = Color(0xFF42705B))
                }
                Spacer(Modifier.weight(1f))
                TextButton(onClick = onManage, contentPadding = PaddingValues(8.dp)) { Text("管理") }
            }
            Text(record.name, fontWeight = FontWeight.Bold, fontSize = 20.sp,
                maxLines = 2, overflow = TextOverflow.Ellipsis)
            Text(record.address, style = MaterialTheme.typography.bodySmall, maxLines = 1,
                overflow = TextOverflow.Ellipsis, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("已保存 · ${record.username}", Modifier.weight(1f), maxLines = 1,
                    overflow = TextOverflow.Ellipsis, style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant)
                Text("›", Modifier.padding(start = 12.dp), style = MaterialTheme.typography.headlineSmall)
            }
        }
    }
}
