package com.kivo.cinema.sources.ui.cards

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/** Shared grid geometry for storage entrances and connected servers. */
@OptIn(androidx.compose.foundation.ExperimentalFoundationApi::class)
@Composable
internal fun SourceTile(title: String, subtitle: String, onOpen: () -> Unit,
    onManage: (() -> Unit)? = null, icon: @Composable () -> Unit,
    accessory: @Composable RowScope.() -> Unit = {}) {
    Surface(modifier = Modifier.combinedClickable(onClick = onOpen, onLongClick = onManage,
        onLongClickLabel = onManage?.let { "管理$title" }),
        shape = RoundedCornerShape(20.dp), border = BorderStroke(.5.dp, Color(0xFFDFDFE6)),
        color = Color.White.copy(alpha = .9f)) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Row(Modifier.fillMaxWidth().heightIn(min = 48.dp), verticalAlignment = Alignment.CenterVertically) {
                icon()
                Spacer(Modifier.weight(1f))
                accessory()
            }
            Text(title, fontSize = 14.sp, lineHeight = 18.sp, maxLines = 2,
                overflow = TextOverflow.Ellipsis)
            Text(subtitle, fontSize = 11.sp, lineHeight = 15.sp, maxLines = 2,
                overflow = TextOverflow.Ellipsis, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
    }
}
