package com.kivo.cinema.ui.search.browse

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.search.SearchTokens

/** ③④ 网格海报卡：2:3 + 片名 +「★ 评分 · 年份」；4K/HDR 角标只在本地真实命中时出现。 */
@Composable
internal fun BrowsePosterCard(entry: SearchEntry, onOpen: (SearchEntry) -> Unit) {
    val shape = RoundedCornerShape(V6Metrics.rPoster)
    Column(Modifier.clip(shape).clickable { onOpen(entry) }) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f).clip(shape)) {
            TmdbImage(entry.posterUrl.orEmpty(), entry.title, Modifier.fillMaxSize())
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
            entry.quality?.let { badge ->
                Text(
                    badge, fontSize = 9.sp, fontWeight = FontWeight.Bold, letterSpacing = .4.sp,
                    color = if (badge == "HDR") SearchTokens.hdrText else DesignTokens.canvasFill,
                    modifier = Modifier.align(Alignment.TopEnd).padding(6.dp)
                        .clip(RoundedCornerShape(5.dp))
                        .background(V6Metrics.badgeGlass)
                        .padding(horizontal = 5.dp, vertical = 2.dp),
                )
            }
        }
        Spacer(Modifier.height(7.dp))
        Text(
            entry.title, color = DesignTokens.textMain, fontSize = 13.sp, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis,
        )
        Row(Modifier.padding(top = 2.dp), verticalAlignment = Alignment.CenterVertically) {
            Text("★", color = V6Metrics.brass, fontSize = 10.sp)
            Text(
                " %.1f · %s".format(entry.rating, entry.year), color = DesignTokens.textSub,
                fontSize = 11.sp, maxLines = 1,
            )
        }
    }
}

/** ③④ 网格尾部：加载 / 失败重试 / 空 / 到底，全部有可见文案。 */
@Composable
internal fun BrowseFooter(
    loading: Boolean,
    empty: Boolean,
    reachedEnd: Boolean,
    error: String?,
    count: Int,
    onRetry: () -> Unit,
    notice: String? = null,
) {
    val text = when {
        error != null && empty -> "$error，点这里重试"
        loading -> if (empty) "正在加载…" else "正在加载更多…"
        empty && reachedEnd -> "这个条件下暂时没有内容"
        reachedEnd -> "已显示全部 $count 部"
        else -> null
    }
    if (text == null && notice == null) return
    Column(Modifier.fillMaxWidth().padding(vertical = 20.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        if (text != null) Text(
            text, fontSize = 13.sp, color = DesignTokens.textSub, textAlign = TextAlign.Center,
            modifier = Modifier.then(if (error != null && empty) Modifier.clickable(onClick = onRetry) else Modifier),
        )
        if (notice != null) Text(
            notice, fontSize = 11.sp, color = DesignTokens.textSub, textAlign = TextAlign.Center,
            modifier = Modifier.padding(top = 6.dp, start = 24.dp, end = 24.dp),
        )
    }
}