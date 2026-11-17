package com.kivo.cinema.library

import android.graphics.Bitmap
import android.provider.MediaStore
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.ProfileNames
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 本地影片海报墙（SU-01 收口）：2 列白卡 + 16dp 圆角 + 去扩展名 + IO 异步缩略图。
 *
 * 单一职责：只负责「可见条目 → 网格卡片」的渲染，权限/查询/私密标记由宿主负责。
 * 缩略图走 produceState + IO，组合期不再同步解码（主线程 IO 修复）。
 */
@Composable
internal fun PosterWall(
    visible: List<MediaItem>,
    onOpen: (MediaItem) -> Unit,
    onLongClick: (MediaItem) -> Unit,
) {
    LazyVerticalGrid(
        columns = GridCells.Fixed(2),
        verticalArrangement = Arrangement.spacedBy(12.dp),
        horizontalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        items(visible) { item ->
            PosterCard(item, onLongClick = { onLongClick(item) }) { onOpen(item) }
        }
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun PosterCard(item: MediaItem, onLongClick: () -> Unit, onClick: () -> Unit) {
    val title = ProfileNames.fileNameOf(item.title) ?: item.title
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .combinedClickable(onClick = onClick, onLongClick = onLongClick)
            .padding(8.dp),
    ) {
        val bitmap by produceState<Bitmap?>(initialValue = null, item.path) {
            value = withContext(Dispatchers.IO) {
                android.media.ThumbnailUtils.createVideoThumbnail(
                    item.path, MediaStore.Video.Thumbnails.MINI_KIND)
            }
        }
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .aspectRatio(2f / 3f)
                .clip(RoundedCornerShape(12.dp))
                .background(DesignTokens.washFill),
        ) {
            val thumb = bitmap
            if (thumb != null) {
                Image(
                    bitmap = thumb.asImageBitmap(),
                    contentDescription = title,
                    contentScale = ContentScale.Crop,
                    modifier = Modifier.fillMaxSize(),
                )
            }
            Text(
                item.formatDuration(),
                fontSize = 10.sp,
                color = androidx.compose.ui.graphics.Color.White,
                modifier = Modifier
                    .align(Alignment.BottomEnd)
                    .padding(6.dp)
                    .background(androidx.compose.ui.graphics.Color.Black.copy(alpha = 0.6f),
                        RoundedCornerShape(4.dp))
                    .padding(horizontal = 4.dp, vertical = 1.dp),
            )
        }
        Spacer(Modifier.height(6.dp))
        Text(
            title,
            fontSize = 13.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.Start,
            modifier = Modifier.fillMaxWidth(),
        )
        Text(
            formatDate(item.addedSec),
            fontSize = DesignTokens.fontMeta.sp,
            color = DesignTokens.textSub,
            textAlign = TextAlign.Start,
            modifier = Modifier.fillMaxWidth(),
        )
    }
}
