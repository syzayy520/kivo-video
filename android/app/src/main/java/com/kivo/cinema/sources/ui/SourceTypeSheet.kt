package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.sources.ui.cards.SourceIcon
import com.kivo.cinema.sources.ui.cards.SourceIconKind

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceTypeSheet(onDismiss: () -> Unit, onEmby: () -> Unit, onSmb: () -> Unit) {
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true),
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        SourceTypeContent(onDismiss, onEmby, onSmb)
    }
}

/** Reservations never open a pretend login form. */
@Composable
internal fun SourceTypeContent(onDismiss: () -> Unit, onEmby: () -> Unit, onSmb: () -> Unit) {
    Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState()).padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("添加资源库", Modifier.weight(1f), fontSize = 18.sp)
            TextButton(onClick = onDismiss) { Text("关闭") }
        }
        Text("媒体服务器", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Surface(shape = MaterialTheme.shapes.medium) {
            Column {
                Provider("Emby", SourceIconKind.EMBY, onEmby)
                Provider("Plex", SourceIconKind.PLEX)
                Provider("Jellyfin", SourceIconKind.JELLYFIN)
                Provider("飞牛影视", SourceIconKind.STORAGE)
            }
        }
        Text("文件存储", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Surface(shape = MaterialTheme.shapes.medium) {
            Column {
                Provider("SMB", SourceIconKind.SMB, onSmb)
                Provider("WebDAV", SourceIconKind.WEBDAV)
            }
        }
    }
}

@Composable
private fun Provider(name: String, icon: SourceIconKind, action: (() -> Unit)? = null) {
    Surface(onClick = { action?.invoke() }, enabled = action != null,
        color = MaterialTheme.colorScheme.surface) {
        Row(Modifier.fillMaxWidth().heightIn(min = 60.dp).padding(horizontal = 16.dp, vertical = 8.dp),
            verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            SourceIcon(icon)
            Text(name, Modifier.weight(1f), fontSize = 14.sp)
            Text(if (action == null) "暂未接入" else "›", fontSize = 12.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
    }
}
