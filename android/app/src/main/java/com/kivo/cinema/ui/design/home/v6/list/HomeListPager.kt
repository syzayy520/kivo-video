package com.kivo.cinema.ui.design.home.v6.list

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbList
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 「查看全部」的分页状态：逐页拉取 [TmdbList]，按 id 去重（TMDB 翻页时偶有重复条目），记录加载中 / 失败原因 / 到底。
 *
 * 失败不吞：[error] 为 TmdbHome 的错误类型键（timeout / offline / …），界面用 TmdbErrorCard 显示五类原文并提供重试。
 */
internal class HomeListPager(
    private val home: TmdbHome,
    private val kind: TmdbList,
    private val scope: CoroutineScope,
) {
    var items by mutableStateOf<List<TmdbHome.Item>>(emptyList())
        private set
    var loading by mutableStateOf(false)
        private set
    var error by mutableStateOf<String?>(null)
        private set
    var reachedEnd by mutableStateOf(false)
        private set
    private var nextPage = 1

    /** 拉下一页；正在加载或已到底时忽略（滚动到底会被频繁调用）。 */
    fun loadNext() {
        if (loading || reachedEnd || nextPage > MAX_PAGES) return
        loading = true
        error = null
        scope.launch {
            val data = withContext(Dispatchers.IO) { home.list(kind, nextPage) }
            val seen = items.mapTo(HashSet()) { it.id }
            val fresh = data.items.filter { seen.add(it.id) }
            when {
                data.items.isEmpty() && data.errorKind != null -> error = data.errorKind
                else -> {
                    items = items + fresh
                    nextPage++
                    if (data.items.isEmpty() || nextPage > MAX_PAGES) reachedEnd = true
                }
            }
            loading = false
        }
    }

    private companion object {
        /** 最多 5 页（约 100 部）：再往下 TMDB 的热度排序已没有参考价值，也避免无限请求。 */
        const val MAX_PAGES = 5
    }
}
