package com.kivo.cinema.emby.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyCatalog
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.EmbySessionModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 255：首页按规格一比一——服务器自有分类（/Views）各渲染一条横排海报（名称 + › 钻取）。
 * 电视/电影/动漫/纪录片都由服务器 CollectionType 决定，不在客户端硬编码分类。
 * 横排首屏懒加载（进入视口才拉），避免一次打满十几个分类请求。
 */
@Composable
fun EmbyBrowseScreen(
    model: EmbySessionModel, onBack: () -> Unit,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
) {
    Column(Modifier.fillMaxSize()) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onBack) { Text("‹  返回", fontSize = 16.sp) }
            Spacer(Modifier.weight(1f))
            TextButton(onClick = onBack) { Text("切换服务器") }
        }
        Column(Modifier.padding(horizontal = 24.dp)) {
            Text(model.serverName, fontSize = 30.sp, fontWeight = FontWeight.Bold)
            Text(if (model.viewsLoaded) "服务器分类 · ${model.views.size} 个媒体库" else "正在读取服务器分类…",
                fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(top = 6.dp))
            Spacer(Modifier.height(12.dp))
        }
        if (!model.viewsLoaded) {
            Box(Modifier.weight(1f).fillMaxWidth(), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.size(28.dp), strokeWidth = 2.dp)
            }
        } else if (model.views.isEmpty()) {
            Box(Modifier.weight(1f).fillMaxWidth().padding(32.dp), contentAlignment = Alignment.Center) {
                Text("服务器没有返回媒体库分类，请检查账号权限。",
                    fontSize = 15.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        } else {
            LazyColumn(Modifier.weight(1f), contentPadding = PaddingValues(bottom = 24.dp)) {
                items(model.views, key = { it.id }) { view ->
                    ViewRow(model, view, onOpenView, onItem)
                }
            }
        }
    }
}

@Composable
private fun ViewRow(
    model: EmbySessionModel, view: EmbyCatalog.View,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
) {
    var items by remember(view.id) { mutableStateOf<List<EmbyItem>?>(null) }
    LaunchedEffect(view.id) { items = withContext(Dispatchers.IO) { model.rowPage(view) } }
    Column(Modifier.fillMaxWidth().padding(vertical = 12.dp)) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 24.dp), verticalAlignment = Alignment.CenterVertically) {
            Text(view.name, fontSize = 20.sp, fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
            TextButton(onClick = { onOpenView(view) }) { Text("›", fontSize = 20.sp) }
        }
        Spacer(Modifier.height(10.dp))
        val list = items
        when {
            list == null -> Box(Modifier.fillMaxWidth().height(210.dp), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.size(22.dp), strokeWidth = 2.dp)
            }
            list.isEmpty() -> Text("该分类暂无内容", fontSize = 13.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(horizontal = 24.dp))
            else -> LazyRow(contentPadding = PaddingValues(horizontal = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                items(list, key = { it.id }) { item ->
                    Box(Modifier.width(128.dp)) { EmbyPosterCard(model, item) { onItem(item) } }
                }
            }
        }
    }
}