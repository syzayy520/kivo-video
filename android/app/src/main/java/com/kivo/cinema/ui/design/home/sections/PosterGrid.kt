package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 海报宽高比 2:3（TMDB 标准海报）✅ 与 `TmdbPosterCard` 的 104×156 一致。 */
private const val PosterAspectRatio = 2f / 3f

/**
 * 5 列海报墙（竞品帧 IMG_0335「分类浏览」下的海报网格）。
 *
 * 分块方式：**手动按 `columns` 切块，每块一行 `Row` + `weight(1f)` 等分**。
 * 不用 `LazyVerticalGrid`——它嵌在首页 `LazyColumn` 里会因无界高度直接抛
 * `IllegalStateException`（"Vertically scrollable component was measured with an
 * infinity maximum height constraints"），这是 Compose 的硬约束而不是风格选择 ✅ 实测已知约束。
 * 末行不足 `columns` 时用 `Spacer(weight = 1f)` 补齐，保证每列宽度与上面的行完全对齐
 * （否则末行海报会因 weight 重新分配而变宽）。
 *
 * 参数：`columns` 默认 5；`items` 为空时**不渲染任何东西**（无空行占位）。
 */
@Composable
fun PosterGrid(
    items: List<TmdbHome.Item>,
    columns: Int = 5,
    fg: Color,
    onOpen: (TmdbHome.Item) -> Unit,
) {
    if (items.isEmpty()) return
    val cols = columns.coerceAtLeast(1)
    Column(
        Modifier.fillMaxWidth().padding(horizontal = 13.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        items.chunked(cols).forEach { rowItems ->
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                rowItems.forEach { item ->
                    PosterGridCell(item, fg, onOpen, Modifier.weight(1f))
                }
                repeat(cols - rowItems.size) { Spacer(Modifier.weight(1f)) }
            }
        }
    }
}

@Composable
private fun PosterGridCell(
    item: TmdbHome.Item,
    fg: Color,
    onOpen: (TmdbHome.Item) -> Unit,
    modifier: Modifier,
) {
    Column(modifier.clickable { onOpen(item) }) {
        TmdbImage(
            url = item.posterUrl,
            contentDescription = item.title,
            modifier = Modifier
                .fillMaxWidth()
                .aspectRatio(PosterAspectRatio)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp)),
        )
        Spacer(Modifier.height(4.dp))
        Text(
            item.title,
            fontSize = 11.sp,
            color = fg,
            maxLines = 2,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

private fun previewItem(id: Int, title: String) = TmdbHome.Item(
    id = id,
    title = title,
    date = "2026-11-05",
    overview = "",
    genres = listOf("科幻"),
    rating = 8.1f,
    posterUrl = "",
    posterHeroUrl = "",
    backdropUrl = "",
    isTv = false,
)

@Preview(name = "PosterGrid 空 / 2 条", widthDp = 390, heightDp = 320, showBackground = true)
@Composable
private fun PosterGridPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        PosterGrid(emptyList(), fg = DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        PosterGrid(
            items = listOf(previewItem(1, "沙丘：救世主"), previewItem(2, "银翼杀手 2099")),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}