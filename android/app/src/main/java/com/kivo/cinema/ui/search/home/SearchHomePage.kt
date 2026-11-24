package com.kivo.cinema.ui.search.home

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.CategoryArtworkRepository
import com.kivo.cinema.search.HotSearchRepository
import com.kivo.cinema.search.SearchCategory
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.search.SearchTokens
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * ① 搜索首页编排：大标题 → 搜索框入口 → 最近搜索 → 六分类 → 热搜榜。
 * 数据加载在 IO 线程（热搜/分类剧照各走 12h 缓存）；失败只在该模块内可见，不拖垮整页。
 */
@Composable
internal fun SearchHomePage(
    recent: List<String>,
    hot: HotSearchRepository,
    artwork: CategoryArtworkRepository,
    onOpenQuery: () -> Unit,
    onVoice: () -> Unit,
    onRecentSelect: (String) -> Unit,
    onClearRecent: () -> Unit,
    onCategory: (SearchCategory) -> Unit,
    onHotSelect: (SearchEntry) -> Unit,
) {
    var hotVersion by remember { mutableStateOf(0) }
    val board by produceState<HotSearchRepository.Board?>(null, hot, hotVersion) {
        value = withContext(Dispatchers.IO) { hot.load() }
    }
    val artworks by produceState(emptyMap<SearchCategory, String>(), artwork) {
        value = withContext(Dispatchers.IO) { artwork.load() }
    }
    val loaded = board

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(bottom = SearchTokens.navBottomSpace),
    ) {
        item {
            Text(
                "搜索", fontSize = 34.sp, fontWeight = FontWeight.Bold, letterSpacing = .3.sp,
                color = DesignTokens.textMain,
                modifier = Modifier.padding(start = SearchTokens.pad, end = SearchTokens.pad, top = 6.dp, bottom = 12.dp),
            )
        }
        item { SearchFieldEntry(onOpenQuery, onVoice) }
        item {
            Column(Modifier.padding(top = SearchTokens.sectionGap)) {
                RecentSearchChips(recent, onRecentSelect, onClearRecent)
            }
        }
        item {
            Column(Modifier.padding(top = SearchTokens.sectionGap)) {
                Text(
                    "分类", fontSize = 20.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                    modifier = Modifier.padding(start = SearchTokens.pad, end = SearchTokens.pad, bottom = 10.dp),
                )
                CategoryCardGrid(artworks, onCategory)
            }
        }
        item {
            Spacer(Modifier.height(SearchTokens.sectionGap))
            HotSearchBoard(
                items = loaded?.items.orEmpty(),
                loading = loaded == null,
                error = loaded?.error,
                onRetry = { hotVersion++ },
                onSelect = onHotSelect,
            )
        }
    }
}
/** 门禁 Preview：只预览静态 chrome（搜索框 + 最近搜索 chips），不触网。 */
@Preview
@Composable
private fun SearchHomeChromePreview() {
    Column {
        SearchFieldEntry(onOpen = {}, onVoice = {})
        RecentSearchChips(listOf("黑帮领地", "诺兰"), onSelect = {}, onClear = {})
    }
}