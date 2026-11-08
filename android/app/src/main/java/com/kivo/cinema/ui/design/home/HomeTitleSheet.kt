package com.kivo.cinema.ui.design.home

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens

/** Shows provider metadata without inventing a playable local source. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun HomeTitleSheet(item: TmdbHome.Item, onDismiss: () -> Unit) {
    ModalBottomSheet(onDismissRequest = onDismiss, containerColor = DesignTokens.pageBackground) {
        Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState())
            .padding(horizontal = 22.dp).padding(bottom = 32.dp)) {
            Text(item.title, fontSize = 24.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            Spacer(Modifier.height(8.dp))
            Text(item.metaLine, fontSize = 13.sp, color = DesignTokens.textSub)
            Spacer(Modifier.height(8.dp))
            Text("TMDB · %.1f".format(item.rating), fontSize = 14.sp, color = DesignTokens.textMain)
            Spacer(Modifier.height(18.dp))
            Text(item.overview.ifBlank { "暂无简介" }, fontSize = 15.sp, lineHeight = 24.sp,
                color = DesignTokens.textMain)
            Spacer(Modifier.height(18.dp))
            TextButton(onClick = onDismiss) { Text("关闭") }
        }
    }
}
