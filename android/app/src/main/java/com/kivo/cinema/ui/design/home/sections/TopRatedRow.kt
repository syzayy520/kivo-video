package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 冻结契约：固定宽 64.7dp（REX `rex-dna-browse.md` §1.1 实测 64.7~68），2:3 ⇒ 高 97.05dp。 */
private val TopRatedPosterWidth = 64.7.dp
private val TopRatedPosterAspect = 2f / 3f

// 与 `GenrePosterWall` **完全一致**的横滑口径：左 13dp（REX 发现页页边距）、
// 列间距 15.7dp（`rex-dna-browse.md` §1.1）、末项右侧 13dp —— 390pt 基准下第 5 张溢出屏缘 9.3dp，
// 因此右侧始终有可视 peek 提醒「可横滑」，与 REX 的 5.4 列观感一致。
private val TopRatedSidePad = 13.dp
/** 右侧额外留白：K40（411dp）下 13dp 会让第 5 张整屏、peek 归零；26dp 保住约 1/4 张可见。 */
private val TopRatedEndPad = 26.dp
private val TopRatedGap = 15.7.dp

/**
 * 高分剧集海报横滑行（竞品帧 `rex-genre-grid.jpg` 的「高分剧集」）。
 *
 * 冻结契约（B-3b 裁决：按 REX 横滑，见 `kivo-play-baseline.md` C4 同批口径）：
 * - `LazyRow` + `Arrangement.spacedBy(15.7dp)`，`contentPadding` 左右各 13dp；
 * - 单张海报**固定宽 64.7dp**、`aspectRatio(2f/3f)` ⇒ 高 97.05dp、圆角 `DesignTokens.radiusPoster`；
 * - 一屏（390pt 基准）完整可见 4 张，第 5 张露出 55.4dp（占卡宽 85.6%），横向可滑 22.3dp；
 * - **只画 2:3 海报，不画标题、不画日期**（上一版挂两行文字导致该区高于上方海报墙，已删除）；
 * - **不画节大标题**：由 `TopRatedSection` 绘制。
 *
 * 为什么用 `LazyRow` 而不是 `chunked(5) + Row`（B-3b 前实现）：
 * - 旧实现的 `weight(1f)` 是**等分自适应**，390pt 下卡宽只有 60.2dp，与 REX 实测 64.7 差 4.5dp；
 *   宽屏（如 K40 411dp）下更会被拉到 65.7dp，尺寸随设备漂移、无法与竞品并排对齐；
 * - 固定宽 + 横滑是 REX 的真实交互：第 5 张故意溢出屏缘形成 peek，用户一眼知道还能滑。
 *   等分网格没有 peek，信息密度在窄屏上被动缩水。
 * - `LazyRow` 可横向嵌套在首页 `LazyColumn` 内（纵向嵌套才会因无界高度抛 `IllegalStateException`），
 *   旧 KDoc 里那条「不用 Lazy*」的约束只针对 `LazyVerticalGrid`。
 *
 * 参数 `fg`：仅用于加载中/失败时的占位底色（`fg` 8% 叠色，与 `GenrePosterWall` 同一做法）。
 * 空列表：不渲染任何东西（`LazyRow` 也不创建）。
 */
@Composable
fun TopRatedRow(items: List<TmdbHome.Item>, fg: Color, onOpen: (TmdbHome.Item) -> Unit) {
    if (items.isEmpty()) return
    LazyRow(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(TopRatedGap),
        contentPadding = PaddingValues(start = TopRatedSidePad, end = TopRatedEndPad),
    ) {
        items(items) { item -> TopRatedCell(item, fg, onOpen) }
    }
}

@Composable
private fun TopRatedCell(
    item: TmdbHome.Item,
    fg: Color,
    onOpen: (TmdbHome.Item) -> Unit,
) {
    TmdbImage(
        url = item.posterUrl,
        contentDescription = item.title,
        modifier = Modifier
            .width(TopRatedPosterWidth)
            .aspectRatio(TopRatedPosterAspect)
            .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))
            .clickable { onOpen(item) },
        bg = fg.copy(alpha = PosterPlaceholderAlpha),
        fit = false,
    )
}

private const val PosterPlaceholderAlpha = 0.08f

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

@Preview(name = "TopRatedRow 空 / 7 条（横滑 peek）", widthDp = 390, heightDp = 220, showBackground = true)
@Composable
private fun TopRatedRowPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        TopRatedRow(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        TopRatedRow(
            items = listOf(
                previewItem(1, "最后生还者", "2026-09-12"),
                previewItem(2, "帝王计划", "2026-10-03"),
                previewItem(3, "继承之战", "2026-08-21"),
                previewItem(4, "安多", "2026-07-15"),
                previewItem(5, "幕府将军", "2026-06-02"),
                previewItem(6, "熊家餐馆", "2026-05-11"),
                previewItem(7, "苍穹浩瀚", "2026-04-08"),
            ),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}