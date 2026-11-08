package com.kivo.cinema.sources.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.unit.dp
import com.kivo.cinema.R

/** Protocol choice only. Authentication belongs to the selected provider's form. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceTypeSheet(onDismiss: () -> Unit, onEmby: () -> Unit, onSmb: () -> Unit) {
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        Column(Modifier.fillMaxWidth().padding(horizontal = 20.dp).padding(bottom = 24.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)) {
            Text("添加资源库", style = MaterialTheme.typography.headlineSmall)
            Text("选择来源类型", style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
            Surface(shape = RoundedCornerShape(16.dp)) {
                Column {
                    Surface(onClick = onEmby, color = MaterialTheme.colorScheme.surface) {
                        Row(Modifier.fillMaxWidth().padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
                            Image(painterResource(R.drawable.emby_logo), "Emby", Modifier.size(28.dp))
                            Text("Emby", Modifier.weight(1f).padding(start = 16.dp))
                            Text("›", color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                    Row(Modifier.fillMaxWidth().heightIn(min = 60.dp).padding(horizontal = 16.dp),
                        verticalAlignment = Alignment.CenterVertically) {
                        Text("Plex", Modifier.weight(1f), color = MaterialTheme.colorScheme.onSurfaceVariant)
                        Text("暂未接入", style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                }
            }
            OutlinedButton(onClick = onSmb, modifier = Modifier.fillMaxWidth().heightIn(min = 52.dp)) {
                Text("网络共享 · SMB")
            }
            TextButton(onClick = onDismiss, modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)) { Text("取消") }
        }
    }
}
