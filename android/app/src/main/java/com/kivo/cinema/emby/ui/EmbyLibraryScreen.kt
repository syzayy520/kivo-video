package com.kivo.cinema.emby.ui

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.EmbySessionModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.text.Collator
import java.util.Locale

@Composable
fun EmbyLibraryScreen(model: EmbySessionModel, onBack: () -> Unit, onItem: (EmbyItem) -> Unit) {
    var sorting by remember { mutableStateOf(false) }
    val list = remember(model.items, model.query, model.sort) {
        val matching = model.items.filter { it.name.contains(model.query.trim(), ignoreCase = true) }
        when (model.sort) {
            1 -> matching.sortedWith(compareBy(Collator.getInstance(Locale.CHINA)) { it.name })
            2 -> matching.sortedByDescending { it.year ?: 0 }
            else -> matching
        }
    }
    Column(Modifier.fillMaxSize()) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onBack) { Text("‹  返回", fontSize = 16.sp) }
            Spacer(Modifier.weight(1f))
            TextButton(onClick = onBack) { Text("切换服务器") }
        }
        Column(Modifier.padding(horizontal = 24.dp)) {
            Text("电影", fontSize = 30.sp, fontWeight = FontWeight.Bold)
            Text("${model.serverName} · 已加载 ${model.items.size} 部", fontSize = 13.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.padding(top = 6.dp))
            Spacer(Modifier.height(20.dp))
            OutlinedTextField(value = model.query, onValueChange = { model.query = it },
                placeholder = { Text("搜索已加载的电影") }, singleLine = true,
                shape = RoundedCornerShape(16.dp), modifier = Modifier.fillMaxWidth(),
                trailingIcon = { if (model.query.isNotEmpty()) TextButton(onClick = { model.query = "" }) { Text("清除") } })
            Row(Modifier.fillMaxWidth().padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                Text(if (model.query.isBlank()) "全部已加载电影" else "${list.size} 个搜索结果",
                    fontSize = 14.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.weight(1f))
                Box {
                    TextButton(onClick = { sorting = true }) { Text(listOf("最近添加", "名称", "年份")[model.sort] + " ▾") }
                    DropdownMenu(expanded = sorting, onDismissRequest = { sorting = false }) {
                        listOf("最近添加", "名称", "年份").forEachIndexed { index, name ->
                            DropdownMenuItem(text = { Text(if (model.sort == index) "✓  $name" else name) },
                                onClick = { model.sort = index; sorting = false })
                        }
                    }
                }
            }
        }
        if (list.isEmpty()) Box(Modifier.weight(1f).fillMaxWidth().padding(32.dp), contentAlignment = Alignment.Center) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(if (model.items.isEmpty()) "这里还没有电影" else "没有找到匹配的电影", fontSize = 19.sp, fontWeight = FontWeight.SemiBold)
                Spacer(Modifier.height(10.dp))
                Text(if (model.items.isEmpty()) "可以检查服务器里的电影库和账号权限。" else "试试其他片名，搜索范围是当前已加载的列表。",
                    color = MaterialTheme.colorScheme.onSurfaceVariant, fontSize = 14.sp)
                if (model.query.isNotBlank()) TextButton(onClick = { model.query = "" }) { Text("清除搜索") }
            }
        } else key(model.sort, model.query) {
            LazyVerticalGrid(columns = GridCells.Adaptive(128.dp), modifier = Modifier.weight(1f),
                contentPadding = PaddingValues(start = 24.dp, end = 24.dp, bottom = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp), verticalArrangement = Arrangement.spacedBy(22.dp)) {
                items(list, key = { it.id }) { item -> PosterCard(model, item) { onItem(item) } }
            }
        }
    }
}

@Composable
private fun PosterCard(model: EmbySessionModel, item: EmbyItem, onClick: () -> Unit) {
    var bitmap by remember(item.posterUrl) { mutableStateOf<Bitmap?>(null) }
    LaunchedEffect(item.posterUrl) { bitmap = withContext(Dispatchers.IO) { model.poster(item) } }
    Column(Modifier.clip(RoundedCornerShape(12.dp)).clickable(onClick = onClick)) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f).clip(RoundedCornerShape(12.dp))
            .background(MaterialTheme.colorScheme.primary.copy(alpha = .06f)), contentAlignment = Alignment.Center) {
            if (bitmap != null) Image(bitmap!!.asImageBitmap(), contentDescription = null,
                contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
            else Text("暂无海报", fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        Text(item.name, fontSize = 14.sp, lineHeight = 20.sp, fontWeight = FontWeight.Medium,
            maxLines = 2, minLines = 2, overflow = TextOverflow.Ellipsis,
            modifier = Modifier.padding(top = 9.dp))
        Text(item.year?.toString() ?: "年份未知", fontSize = 12.sp,
            color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.padding(top = 3.dp))
    }
}
