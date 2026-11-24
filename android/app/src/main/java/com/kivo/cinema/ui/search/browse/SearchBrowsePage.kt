package com.kivo.cinema.ui.search.browse

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.grid.rememberLazyGridState
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.BrowseRepository
import com.kivo.cinema.search.SearchCategory
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.history.glassSurface
import com.kivo.cinema.ui.motion.MotionGate
import com.kivo.cinema.ui.search.SearchGlyph
import com.kivo.cinema.ui.search.SearchIcon
import com.kivo.cinema.ui.search.SearchTokens
import com.kivo.cinema.ui.search.filterSummary
import kotlinx.coroutines.launch

/**
 * ③④ 分类筛选页：常驻顶栏（返回 + 当前分类名），tabs/三行筛选/工具行随网格滚走，
 * 滚过筛选区后吸顶摘要接管（④）；点摘要滚回顶部即展开，过渡尊重「减弱动画效果」。
 */
@Composable
internal fun SearchBrowsePage(
    repo: BrowseRepository,
    onBack: () -> Unit,
    onOpen: (SearchEntry) -> Unit,
) {
    val grid = rememberLazyGridState()
    val scope = rememberCoroutineScope()
    val reduce = MotionGate.rememberAppReduceMotion()
    val compact by remember { derivedStateOf { grid.firstVisibleItemIndex > 0 } }
    val nearEnd by remember {
        derivedStateOf {
            (grid.layoutInfo.visibleItemsInfo.lastOrNull()?.index ?: 0) >= repo.items.size - 6
        }
    }
    LaunchedEffect(nearEnd, repo.items.size) {
        if (nearEnd && repo.items.isNotEmpty()) repo.loadNext()
    }

    Box(Modifier.fillMaxSize()) {
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            state = grid,
            contentPadding = PaddingValues(
                start = SearchTokens.pad, end = SearchTokens.pad,
                top = 52.dp, bottom = SearchTokens.navBottomSpace,
            ),
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
        ) {
            item(span = { GridItemSpan(3) }) {
                CategoryTabs(repo.category, repo::selectCategory)
            }
            item(span = { GridItemSpan(3) }) {
                BrowseFilterRows(repo.filter, repo::updateFilter)
            }
            item(span = { GridItemSpan(3) }) {
                BrowseToolRow(repo.displayCountText(), repo.filter, repo::updateFilter)
            }
            items(repo.items, key = { it.key }) { entry ->
                BrowsePosterCard(entry, onOpen)
            }
            item(span = { GridItemSpan(3) }) {
                BrowseFooter(
                    loading = repo.loading,
                    empty = repo.items.isEmpty(),
                    reachedEnd = repo.reachedEnd,
                    error = repo.error,
                    // 「已显示全部 N 部」必须是实际加载条数，不是 TMDB 分页上限的 total。
                    count = repo.items.size,
                    onRetry = { repo.loadNext() },
                    notice = if (repo.filter.onlyPlayable) repo.embyNotice() else null,
                )
            }
        }
        Column(
            Modifier.align(Alignment.TopCenter).fillMaxWidth()
                // 顶栏背景必须常驻不透明：透明时滚动海报会从「电影」标题下穿过去。
                .background(if (compact) SearchTokens.stickyFill else DesignTokens.pageBackground),
        ) {
            BrowseTopBar(repo.category.label, onBack)
            AnimatedVisibility(
                visible = compact,
                enter = if (reduce) fadeIn(tween(0)) else fadeIn(tween(180)) + expandVertically(tween(180)),
                exit = if (reduce) fadeOut(tween(0)) else fadeOut(tween(150)) + shrinkVertically(tween(150)),
            ) {
                StickySummaryPill(filterSummary(repo.filter), repo.displayCountText()) {
                    scope.launch { grid.animateScrollToItem(0) }
                }
            }
        }
    }
}

/** 顶栏：玻璃圆形返回 + 居中分类名（稿面 .bar3）。 */
@Composable
private fun BrowseTopBar(title: String, onBack: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().height(52.dp).padding(horizontal = SearchTokens.pad, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            Modifier.size(44.dp).glassSurface(CircleShape).clip(CircleShape).clickable(onClick = onBack),
            contentAlignment = Alignment.Center,
        ) {
            SearchGlyph(SearchIcon.Back, DesignTokens.textMain, Modifier.size(19.dp))
        }
        Text(
            title, fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain,
            modifier = Modifier.weight(1f), textAlign = androidx.compose.ui.text.style.TextAlign.Center,
        )
        Spacer44()
    }
}

@Composable
private fun Spacer44() = Box(Modifier.size(44.dp))

/** 一级分类横滑 tabs：当前项加粗 + 短下划线（无大面积底色）。 */
@Composable
private fun CategoryTabs(selected: SearchCategory, onSelect: (SearchCategory) -> Unit) {
    androidx.compose.foundation.lazy.LazyRow(
        contentPadding = PaddingValues(horizontal = SearchTokens.pad),
        horizontalArrangement = Arrangement.spacedBy(22.dp),
    ) {
        items(SearchCategory.entries.size) { index ->
            val category = SearchCategory.entries[index]
            val on = category == selected
            Column(
                Modifier.clip(RoundedCornerShape(6.dp)).clickable { onSelect(category) }
                    .padding(top = 10.dp, bottom = 12.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                Text(
                    category.label, fontSize = if (on) 17.sp else 16.sp,
                    fontWeight = if (on) FontWeight.Bold else FontWeight.Normal,
                    color = if (on) DesignTokens.textMain else DesignTokens.textSub,
                )
                Box(
                    Modifier.padding(top = 5.dp).height(3.dp).width(16.dp)
                        .clip(RoundedCornerShape(2.dp))
                        .background(if (on) DesignTokens.textMain else Color.Transparent),
                )
            }
        }
    }
}
/** 门禁 Preview：顶栏 + tabs + 三行筛选 + 吸顶摘要（静态样例，不触网）。 */
@Preview
@Composable
private fun BrowseChromePreview() {
    val sample = com.kivo.cinema.search.BrowseFilter(genreId = 80, year = "2025")
    Column(Modifier.background(DesignTokens.pageBackground)) {
        BrowseTopBar("电影", {})
        CategoryTabs(SearchCategory.Movie, {})
        BrowseFilterRows(sample, {})
        StickySummaryPill(filterSummary(sample), "128") {}
    }
}