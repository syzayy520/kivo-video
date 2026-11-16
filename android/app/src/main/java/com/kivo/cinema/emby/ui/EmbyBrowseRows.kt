package com.kivo.cinema.emby.ui

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyCatalog
import com.kivo.cinema.emby.EmbyHeroItem
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.EmbySessionModel
import com.kivo.cinema.ui.compose.player.formatPlayerTime
import com.kivo.cinema.ui.design.TmdbImageCache
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 317 · 服务器首页的两条横排：**继续观看** 与 **分类行**。
 *
 * 单独一个文件的原因：它们各自有独立的数据加载与交互（继续观看要算进度条、
 * 分类行要懒加载首屏），与页面骨架/顶部栏是三种不同职责。
 */

/** 「继续观看」横排：16:9 缩略图 + 底部进度条 + 「看到 xx:xx」。 */
@Composable
internal fun EmbyResumeRow(items: List<EmbyHeroItem>, onOpen: (EmbyHeroItem) -> Unit) {
    Column(Modifier.fillMaxWidth().padding(top = 16.dp)) {
        Text(
            "继续观看", fontSize = 18.sp, fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(horizontal = 24.dp, vertical = 6.dp),
        )
        LazyRow(
            contentPadding = PaddingValues(horizontal = 24.dp),
            horizontalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            items(items, key = { it.id }) { h ->
                Column(Modifier.width(208.dp)) {
                    Box(
                        Modifier.fillMaxWidth().aspectRatio(16f / 9f)
                            .clip(RoundedCornerShape(10.dp))
                            .background(MaterialTheme.colorScheme.primary.copy(alpha = .06f)),
                    ) {
                        EmbyHeroThumb(h.heroUrl, h.name)
                        val frac = if (h.runtimeMs > 0) {
                            (h.resumeMs.toFloat() / h.runtimeMs).coerceIn(0f, 1f)
                        } else 0f
                        if (frac > 0f) {
                            Box(
                                Modifier.align(Alignment.BottomStart).fillMaxWidth(frac)
                                    .height(3.dp).background(MaterialTheme.colorScheme.primary),
                            )
                        }
                    }
                    Text(h.name, fontSize = 13.sp, maxLines = 1, modifier = Modifier.padding(top = 7.dp))
                    Text(
                        "看到 " + formatPlayerTime(h.resumeMs),
                        fontSize = 11.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(top = 2.dp),
                    )
                }
            }
        }
    }
}

/**
 * 一个媒体库分类（标题 + 横排海报）。
 *
 * 空分类**整行不渲染**（连标题一起）：之前只隐藏内容，标题还立在那里，
 * 18 个媒体库里能看到一个空的「播放列表」，很扎眼。
 */
@Composable
internal fun EmbyCategoryRow(
    model: EmbySessionModel,
    view: EmbyCatalog.View,
    onOpenView: (EmbyCatalog.View) -> Unit,
    onItem: (EmbyItem) -> Unit,
) {
    var items by remember(view.id) { mutableStateOf<List<EmbyItem>?>(null) }
    LaunchedEffect(view.id) { items = withContext(Dispatchers.IO) { model.rowPage(view) } }
    if (items?.isEmpty() == true) return
    Column(Modifier.fillMaxWidth().padding(vertical = 12.dp)) {
        Row(
            Modifier.fillMaxWidth().padding(horizontal = 24.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(view.name, fontSize = 20.sp, fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
            TextButton(onClick = { onOpenView(view) }) { Text("›", fontSize = 20.sp) }
        }
        Spacer(Modifier.height(10.dp))
        when (val list = items) {
            null -> Box(Modifier.fillMaxWidth().height(210.dp), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.size(22.dp), strokeWidth = 2.dp)
            }
            else -> LazyRow(
                contentPadding = PaddingValues(horizontal = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp),
            ) {
                items(list, key = { it.id }) { item ->
                    Box(Modifier.width(128.dp)) { EmbyPosterCard(model, item) { onItem(item) } }
                }
            }
        }
    }
}

/**
 * 横排缩略图。
 *
 * 用通用 loader（无鉴权路径）：hero 主图已验证这些 URL 不带 token 也能取到，
 * 而且能复用它的内存 + 磁盘双缓存，滚动回来不用重下。
 */
@Composable
private fun EmbyHeroThumb(url: String, name: String) {
    var bmp by remember(url) { mutableStateOf<android.graphics.Bitmap?>(null) }
    LaunchedEffect(url) {
        bmp = withContext(Dispatchers.IO) {
            if (url.isBlank()) null else TmdbImageCache.load(url)
        }
    }
    val frame = bmp
    if (frame != null) {
        Image(
            bitmap = frame.asImageBitmap(),
            contentDescription = name,
            contentScale = ContentScale.Crop,
            modifier = Modifier.fillMaxSize(),
        )
    }
}
