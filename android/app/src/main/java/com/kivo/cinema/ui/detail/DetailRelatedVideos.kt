package com.kivo.cinema.ui.detail

import android.content.Context
import android.graphics.Bitmap
import android.provider.MediaStore as MS
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.LocalVideoSource
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.ui.design.DesignTokens
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 详情页「同目录」区（W1）——同文件夹的其它片子，2 列 2:3 卡，最多 6 条。
 *
 * ## 冻结契约
 * `DetailRelatedVideos(items: List<MediaItem>, onOpen: (MediaItem) -> Unit)`；
 * `items` 为空时**整段不渲染**（含 12dp 顶部外距），页面上不留空白或「暂无」文案。
 * `onOpen` 由调用方决定怎么打开（详情页当前是再起一个详情 Activity）。
 *
 * ## 同目录查询口径（[queryRelatedVideos]）
 * · 父目录 = 路径最后一个 `/` 之前的部分。`MediaStore.RELATIVE_PATH` 的相对目录与
 *   `DATA` 的父目录是同一层语义，用 `DATA` 比较不必额外请求权限、也不受 API 29 前后
 *   `RELATIVE_PATH` 是否存在的差异影响。
 * · 排除自身按**完整路径**（同一个文件可能被 MediaStore 索引两次也只应出现一次）。
 * · 排序按 `DATE_ADDED` 倒序（新加入的在前），取前 [DEFAULT_LIMIT] 条。
 * · 任何异常（无权限 / 查询失败 / 路径无父目录 / 空路径）一律返回空表 ⇒ 页面不画这一段。
 *
 * ## 视觉参数与偏差
 * 标题 16sp Bold；卡标题 13sp Bold 单行、时长 11sp `textSub`；行内 12dp、列间 10dp。
 * 媒体槽 2:3 圆角（`DesignTokens.radiusPoster`）、底色 `groupedFill`、封面 `Crop`。
 * **⚠️ 显式偏差**：任务基准写「104×156 级」，那是 3 列基准；本页固定 2 列，
 * 若用固定 104dp 宽会在 334dp 内容宽里剩 116dp 空白（13dp 屏边 × 2），因此改为
 * `weight(1f) + aspectRatio(2/3)`：360dp 屏每格实测约 161×241，320dp 屏约 141×211。
 * 视觉密度优先于字面尺寸（本卡的目的正是补足底部空白）。
 * · 缩略图缺失只留 `groupedFill` 底，不写占位文字、不用裸色。
 * · 缩略图在 IO 线程生成（`produceState`），组合期不阻塞主线程。
 */
@Composable
fun DetailRelatedVideos(items: List<MediaItem>, onOpen: (MediaItem) -> Unit) {
    if (items.isEmpty()) return
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = GapBefore),
    ) {
        Text(
            "同目录",
            fontSize = 16.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        Spacer(Modifier.height(TitleGap))
        items.take(DEFAULT_LIMIT).chunked(Columns).forEachIndexed { index, rowItems ->
            if (index > 0) Spacer(Modifier.height(RowGap))
            Row(horizontalArrangement = Arrangement.spacedBy(ColumnGap)) {
                rowItems.forEach { item ->
                    RelatedVideoCard(item, onOpen, Modifier.weight(1f))
                }
                // 奇数条时补一个占位权重，让最后一张卡保持半宽而不是被拉伸
                repeat(Columns - rowItems.size) { Spacer(Modifier.weight(1f)) }
            }
        }
    }
}

@Composable
private fun RelatedVideoCard(
    item: MediaItem,
    onOpen: (MediaItem) -> Unit,
    modifier: Modifier,
) {
    Column(modifier.clickable(role = Role.Button) { onOpen(item) }) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .aspectRatio(PosterRatio)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))
                .background(DesignTokens.groupedFill),
        ) {
            RelatedThumb(path = item.path, modifier = Modifier.fillMaxSize())
        }
        Spacer(Modifier.height(TitleGap))
        Text(
            text = item.title,
            fontSize = 13.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
        Text(
            text = item.formatDuration(),
            fontSize = 11.sp,
            color = DesignTokens.textSub,
            maxLines = 1,
        )
    }
}

/** 缩略图：IO 线程取 MINI_KIND，失败就什么都不画（底下的 `groupedFill` 就是如实空态）。 */
@Composable
private fun RelatedThumb(path: String, modifier: Modifier) {
    val bmp by produceState<Bitmap?>(initialValue = null, path) {
        value = withContext(Dispatchers.IO) {
            runCatching {
                android.media.ThumbnailUtils.createVideoThumbnail(path, MS.Video.Thumbnails.MINI_KIND)
            }.getOrNull()
        }
    }
    bmp?.let { bitmap ->
        Image(
            bitmap = bitmap.asImageBitmap(),
            contentDescription = null,
            contentScale = ContentScale.Crop,
            modifier = modifier,
        )
    }
}

/**
 * 取与 [path] **同父目录**的其它视频，`DATE_ADDED` 倒序，最多 [limit] 条。
 * 查询失败 / 路径无父目录时返回空表（调用方按「没有这一段」渲染）。
 */
fun queryRelatedVideos(
    context: Context,
    path: String,
    limit: Int = DEFAULT_LIMIT,
): List<MediaItem> {
    val parent = path.substringBeforeLast('/', "")
    if (path.isBlank() || parent.isEmpty() || limit <= 0) return emptyList()
    return LocalVideoSource.query(context)
        .filter { it.path.isNotEmpty() && it.path != path && it.path.substringBeforeLast('/', "") == parent }
        .sortedByDescending { it.addedSec }
        .take(limit)
}

private const val Columns = 2
private const val DEFAULT_LIMIT = 6
private const val PosterRatio = 2f / 3f
private val GapBefore = 12.dp
private val TitleGap = 8.dp
private val ColumnGap = 10.dp
private val RowGap = 12.dp

// ───────────────────────────── Preview ─────────────────────────────

private fun previewItems(): List<MediaItem> = listOf(
    MediaItem(1, "Interstellar.2014.2160p", "/sdcard/Movies/sf/a.mp4", 10_080_000, 3840, 2160, 1),
    MediaItem(2, "Arrival.2016.1080p", "/sdcard/Movies/sf/b.mkv", 6_960_000, 1920, 1080, 2),
    MediaItem(3, "Blade.Runner.2049", "/sdcard/Movies/sf/c.mp4", 9_840_000, 1920, 1080, 3),
)

/** 常态：3 条 ⇒ 两行（第二行只有一张，右侧留半宽空位）。 */
@Preview(widthDp = 360, heightDp = 560)
@Composable
private fun DetailRelatedVideosPreview() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(DesignTokens.screenPad.dp)) {
        DetailRelatedVideos(items = previewItems(), onOpen = {})
    }
}

/** 空态：`items = emptyList()` ⇒ 整段不渲染，只剩 13dp 屏边。 */
@Preview(widthDp = 360, heightDp = 40)
@Composable
private fun DetailRelatedVideosEmptyPreview() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(DesignTokens.screenPad.dp)) {
        DetailRelatedVideos(items = emptyList(), onOpen = {})
    }
}