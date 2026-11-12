package com.kivo.cinema.emby.aggregate

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.EmbyErrors
import com.kivo.cinema.emby.EmbyItem
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 256 跨源合并搜索：把六排筛选翻译成每源查询，并行拉取、按(源+条目)去重、按当前排序键全局合并、
 * 客户端地区过滤、逐源分页。主线程态 + generation 栅栏迟到回包。单一职责：搜索态机，不连接（在 AggregateSession）。
 */
class AggregateSearchModel(private val scope: CoroutineScope) {
    data class Result(val source: AggregateSource, val item: EmbyItem)
    private class PageState(var offset: Int = 0, var exhausted: Boolean = false)

    var query by mutableStateOf(""); private set
    var categoryIndex by mutableStateOf(0); private set
    var sortingIndex by mutableStateOf(0); private set
    var genre by mutableStateOf<String?>(null); private set
    var region by mutableStateOf<String?>(null); private set
    var yearIndex by mutableStateOf<Int?>(null); private set
    var selectedIds by mutableStateOf<Set<String>>(emptySet()); private set

    var results by mutableStateOf<List<Result>>(emptyList()); private set
    var loading by mutableStateOf(false); private set
    var error by mutableStateOf<String?>(null); private set
    var hasMore by mutableStateOf(false); private set
    var partialFailure by mutableStateOf<String?>(null); private set

    private var available: List<AggregateSource> = emptyList()
    private val pages = HashMap<String, PageState>()
    private var generation = 0
    private var debounce: Job? = null
    private var work: Job? = null

    fun attach(sources: List<AggregateSource>) {
        available = sources
        selectedIds = sources.map { it.id }.toSet()
        reset(0)
    }

    fun search(v: String) { if (query != v) { query = v; reset(350) } }
    fun setCategory(i: Int) { if (categoryIndex != i) { categoryIndex = i; reset(0) } }
    fun setSorting(i: Int) { if (sortingIndex != i) { sortingIndex = i; reset(0) } }
    fun pickGenre(g: String?) { if (genre != g) { genre = g; reset(0) } }
    fun pickRegion(r: String?) { if (region != r) { region = r; reset(0) } }
    fun setYear(i: Int?) { if (yearIndex != i) { yearIndex = i; reset(0) } }
    fun toggleSource(id: String) {
        selectedIds = if (id in selectedIds) selectedIds - id else selectedIds + id
        reset(0)
    }

    private fun currentQuery() = AggregateFilter.translate(
        query, AggregateFilter.categories[categoryIndex], AggregateFilter.sortings[sortingIndex],
        genre, region, yearIndex?.let { AggregateFilter.yearOptions[it] },
    )

    private fun reset(delayMs: Long) {
        val ticket = ++generation
        debounce?.cancel(); work?.cancel()
        results = emptyList(); pages.clear(); error = null; partialFailure = null
        hasMore = true; loading = true
        debounce = scope.launch {
            if (delayMs > 0) delay(delayMs)
            if (ticket != generation) return@launch
            loading = false
            loadMore()
        }
    }

    fun loadMore() {
        if (loading || !hasMore) return
        val targets = available.filter { it.id in selectedIds && pages[it.id]?.exhausted != true }
        if (targets.isEmpty()) { hasMore = false; loading = false; return }
        val q = currentQuery()
        val ticket = generation
        loading = true; error = null
        work = scope.launch {
            val fetched = targets.map { src ->
                async {
                    withContext(Dispatchers.IO) {
                        val st = pages.getOrPut(src.id) { PageState() }
                        src to runCatching {
                            src.client.items(src.auth, start = st.offset, search = q.search, sort = q.sort,
                                limit = 60, types = q.types, genres = q.genres, years = q.years,
                                maxPremiereDate = q.maxPremiereDate, rich = true)
                        }
                    }
                }
            }.awaitAll()
            if (ticket != generation) return@launch
            val added = mutableListOf<Result>()
            var failure: String? = null
            for ((src, outcome) in fetched) {
                val st = pages.getOrPut(src.id) { PageState() }
                outcome.fold(onSuccess = { page ->
                    st.offset += page.items.size
                    st.exhausted = page.items.isEmpty() ||
                        (page.total?.let { st.offset >= it } ?: (page.items.size < 60))
                    added += page.items.map { Result(src, it) }
                }, onFailure = { failure = EmbyErrors.reason(it); st.exhausted = true })
            }
            val merged = (results + added).distinctBy { it.source.id + ":" + it.item.id }
                .filter { AggregateFilter.matchesRegion(it.item.countries, q.region) }
            results = sortMerged(merged)
            hasMore = targets.any { pages[it.id]?.exhausted != true }
            partialFailure = failure
            if (results.isEmpty() && failure != null) error = failure
            loading = false
        }
    }

    private fun sortMerged(list: List<Result>): List<Result> = when (sortingIndex) {
        2 -> list.sortedByDescending { it.item.rating ?: -1.0 }
        1 -> list.sortedByDescending { it.item.premiereDate }
        else -> list.sortedByDescending { it.item.dateCreated }
    }

    fun clear() {
        ++generation; debounce?.cancel(); work?.cancel()
        available = emptyList(); pages.clear(); selectedIds = emptySet()
        results = emptyList(); loading = false; error = null; partialFailure = null; hasMore = false
    }
}