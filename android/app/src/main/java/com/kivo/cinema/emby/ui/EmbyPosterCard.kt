package com.kivo.cinema.emby.ui

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
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

/** 255：共用海报卡（钻取网格 + 首页分类横排同一份渲染，禁止两份_look-alike_）。 */
@Composable
fun EmbyPosterCard(model: EmbySessionModel, item: EmbyItem, onClick: () -> Unit) {
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