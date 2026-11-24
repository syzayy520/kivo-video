package com.kivo.cinema.search

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbLocale
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/** ③④ discover 路径构造：分类固有约束 × 类型/地区/年份/排序。All 由仓储双路（movie+tv）调用。 */
object BrowsePaths {
    fun discover(category: SearchCategory, filter: BrowseFilter, page: Int, kindOverride: String? = null): String {
        val kind = kindOverride ?: category.discoverKind
        val tv = kind == "tv"
        val sb = StringBuilder("/discover/$kind?language=${TmdbLocale.tag()}&include_adult=false&page=$page")
        val genres = listOfNotNull(
            category.baseGenres.takeIf { it.isNotEmpty() },
            filter.genreId.takeIf { it > 0 }?.toString(),
        )
        if (genres.isNotEmpty()) sb.append("&with_genres=${genres.joinToString(",")}")
        category.originalLanguage?.let { sb.append("&with_original_language=$it") }
        if (filter.region.isNotEmpty()) sb.append("&with_origin_country=${filter.region}")
        val dateKey = if (tv) "first_air_date" else "primary_release_date"
        when (filter.year) {
            "全部" -> {}
            "2010s" -> sb.append("&$dateKey.gte=2010-01-01&$dateKey.lte=2019-12-31")
            "更早" -> sb.append("&$dateKey.lte=2009-12-31")
            else -> sb.append("&${if (tv) "first_air_date_year" else "primary_release_year"}=${filter.year}")
        }
        val sort = SEARCH_SORTS[filter.sortIndex].first
        sb.append("&sort_by=${if (tv && sort.startsWith("primary_release_date")) "first_air_date.desc" else sort}")
        return sb.toString()
    }
}

/**
 * ③④ 分类筛选态机：discover 分页 + 本地可播标注 + 「只看可播」客户端过滤。
 * 单一职责：筛选数据；UI（tabs/三行胶囊/吸顶摘要）在 ui/search/browse/。
 */
class BrowseRepository(
    private val home: TmdbHome,
    private val local: LocalPlayableIndex,
    private val emby: EmbyPlayableIndex,
    private val localPool: () -> List<MediaItem>,
    private val scope: CoroutineScope,
) {
    var category by mutableStateOf(SearchCategory.Movie); private set
    var filter by mutableStateOf(BrowseFilter()); private set
    var items by mutableStateOf<List<SearchEntry>>(emptyList()); private set
    var total by mutableStateOf(0); private set
    var loading by mutableStateOf(false); private set
    var reachedEnd by mutableStateOf(false); private set
    var error by mutableStateOf<String?>(null); private set

    private var page = 0
    private var totalPages = Int.MAX_VALUE
    private var generation = 0
    private var job: Job? = null

    fun open(initial: SearchCategory) {
        category = initial
        filter = BrowseFilter()
        restart()
    }

    fun selectCategory(value: SearchCategory) {
        if (category == value) return
        category = value
        restart()
    }

    fun updateFilter(value: BrowseFilter) {
        if (filter == value) return
        filter = value
        restart()
    }

    private fun restart() {
        ++generation; job?.cancel()
        items = emptyList(); total = 0; page = 0; totalPages = Int.MAX_VALUE
        reachedEnd = false; error = null
        loadNext()
    }

    fun loadNext() {
        if (loading || reachedEnd) return
        val ticket = generation
        val nextPage = page + 1
        val cat = category
        val f = filter
        loading = true
        job = scope.launch {
            val outcome = withContext(Dispatchers.IO) { fetch(cat, f, nextPage) }
            if (ticket != generation) return@launch
            items = (items + outcome.items).distinctBy { it.key }
            total = outcome.total
            totalPages = outcome.totalPages
            page = nextPage
            reachedEnd = outcome.items.isEmpty() || nextPage >= totalPages
            error = outcome.error
            loading = false
        }
    }

    private data class Outcome(
        val items: List<SearchEntry>,
        val total: Int,
        val totalPages: Int,
        val error: String?,
    )

    private fun fetch(cat: SearchCategory, f: BrowseFilter, page: Int): Outcome {
        val pool = localPool()
        fun convert(data: TmdbHome.PagedHomeData): List<SearchEntry> =
            data.items.map { local.fromTmdb(it, pool, embyPool = emby.pool) }
                .filter { !f.onlyPlayable || it.playable }
        if (cat == SearchCategory.All) {
            val movies = home.pagedItems(BrowsePaths.discover(cat, f, page, "movie"))
            val tvs = home.pagedItems(BrowsePaths.discover(cat, f, page, "tv"))
            val merged = interleave(convert(movies), convert(tvs))
            return Outcome(merged, movies.total + tvs.total,
                maxOf(movies.totalPages, tvs.totalPages),
                (movies.errorKind ?: tvs.errorKind)?.let { SearchRepository.errorText(it) })
        }
        val data = home.pagedItems(BrowsePaths.discover(cat, f, page))
        return Outcome(convert(data), data.total, data.totalPages,
            data.errorKind?.let { SearchRepository.errorText(it) })
    }

    private fun interleave(a: List<SearchEntry>, b: List<SearchEntry>): List<SearchEntry> {
        val out = ArrayList<SearchEntry>(a.size + b.size)
        val n = maxOf(a.size, b.size)
        for (i in 0 until n) {
            if (i < a.size) out.add(a[i])
            if (i < b.size) out.add(b[i])
        }
        return out
    }

    /** UI 计数：「只看可播」时远端 total 不代表命中数，显示已加载的可播条数。 */
    fun displayCount(): Int = if (filter.onlyPlayable) items.size else total

    /** 「共 N 部」文案：TMDB discover 的 total 顶到分页上限（>1 万）时显示「N 万+」，不冒充精确总数。 */
    fun displayCountText(): String = when {
        filter.onlyPlayable -> "${items.size}"
        total > 10000 -> "${total / 10000} 万+"
        else -> "$total"
    }

    /** 「只看可播」页脚：Emby 部分源未接入的可见原因（无则 null）。 */
    fun embyNotice(): String? = emby.notice

    /** Emby 池就绪后强制重筛（仅调用方在需要时触发）。 */
    fun refresh() = restart()

    fun close() { ++generation; job?.cancel() }
}