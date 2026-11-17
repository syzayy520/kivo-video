package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens

/** 竞品帧标题实测 20sp Bold，整行居中 ✅ 实测（区别于其余节头左对齐）。 */
private val TopRatedTitleSize = 20.sp
private val TopRatedSidePad = 13.dp
private val TopRatedHeaderTopPad = 13.dp
private val TopRatedHeaderBottomPad = 16.dp

/** 竞品帧该区只露 5 列 × 2 行；多给的条目在此截断，避免页尾无限延长 ✅ 实测。 */
private const val TopRatedMaxVisible = 10

/**
 * 高分剧集节 = **居中标题 + 5 列海报行**（竞品帧 `rex-genre-grid.jpg`）。
 *
 * 为什么不复用 `LazyListScope.homeSection` ◐ 推断：该节头左对齐，而竞品此节标题
 * **整行居中**；两者视觉不同，强行复用会把标题样式泄漏到别的节，故单独成节。
 * 上下留白（13/16dp）与 `homeSection` 保持同距，节奏不变。
 *
 * 标题左右 13dp 与 `TopRatedRow` 的海报左右边距一致，避免居中标题看起来比网格窄。
 *
 * 空列表：不渲染任何东西（含标题）。
 */
@Composable
fun TopRatedSection(
    items: List<TmdbHome.Item>,
    fg: Color,
    onOpen: (TmdbHome.Item) -> Unit,
) {
    if (items.isEmpty()) return
    Column(Modifier.fillMaxWidth()) {
        Spacer(Modifier.height(TopRatedHeaderTopPad))
        Text(
            text = "高分剧集",
            fontSize = TopRatedTitleSize,
            fontWeight = FontWeight.Bold,
            color = fg,
            textAlign = TextAlign.Center,
            maxLines = 1,
            modifier = Modifier.fillMaxWidth().padding(horizontal = TopRatedSidePad),
        )
        Spacer(Modifier.height(TopRatedHeaderBottomPad))
        TopRatedRow(items.take(TopRatedMaxVisible), fg, onOpen)
    }
}

private fun previewItem(id: Int, title: String, date: String) = TmdbHome.Item(
    id = id,
    title = title,
    date = date,
    overview = "",
    genres = listOf("剧情"),
    rating = 9.2f,
    posterUrl = "",
    posterHeroUrl = "",
    backdropUrl = "",
    isTv = true,
)

@Preview(name = "TopRatedSection 空 / 2 条", widthDp = 390, heightDp = 260, showBackground = true)
@Composable
private fun TopRatedSectionPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        TopRatedSection(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        TopRatedSection(
            items = listOf(
                previewItem(1, "最后生还者", "2026-09-12"),
                previewItem(2, "帝王计划", "2026-10-03"),
            ),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}