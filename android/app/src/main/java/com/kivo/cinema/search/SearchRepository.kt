package com.kivo.cinema.search

import android.content.Context
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.aggregate.LocalSearch
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.library.TitleMatch
import com.kivo.cinema.tmdb.TmdbSearch
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * ② 输入中搜索态机（family-tree §四）：350ms 防抖 + generation 栅栏取消迟到回包，
 * 本地（NAS 索引 + MediaStore，私密区由 LocalSearch 排除）∪ TMDB search/multi。
 *
 * 合并口径：TMDB 条目逐条做本地可播匹配；只在本地产出、TMDB 没列出的条目排在最前。
 * 最佳匹配 = 第一条可播，否则第一条；列表展示其余条目。
 */
class SearchRepository(
    private val app: Context,
    private val tmdb: TmdbSearch,
    private val local: LocalPlayableIndex,
    private val emby: EmbyPlayableIndex,
    private val scope: CoroutineScope,
) {
    var query by mutableStateOf(""); private set
    var entries by mutableStateOf<List<SearchEntry>>(emptyList()); private set
    var loading by mutableStateOf(false); private set
    var error by mutableStateOf<String?>(null); private set
    /** 本地侧可见原因（未授权/私密区损坏）；TMDB 无 key 时只出本地结果，不算错误。 */
    var localNotice by mutableStateOf<String?>(null); private set

    private var generation = 0
    private var job: Job? = null

    fun onQuery(value: String) {
        if (query == value) return
        query = value
        val ticket = ++generation
        job?.cancel()
        if (value.isBlank()) {
            entries = emptyList(); loading = false; error = null; localNotice = null
            return
        }
        loading = true; error = null
        job = scope.launch {
            delay(350)
            val outcome = withContext(Dispatchers.IO) { runSearch(value.trim()) }
            if (ticket != generation) return@launch
            entries = outcome.entries
            error = outcome.error
            localNotice = outcome.localNotice
            loading = false
        }
    }

    private data class Outcome(
        val entries: List<SearchEntry>,
        val error: String?,
        val localNotice: String?,
    )

    private fun runSearch(q: String): Outcome {
        val localState = LocalSearch.match(app, q)
        val pool = localState.items
        val remote = tmdb.multi(q)
        val remoteEntries = remote.items.map { local.fromTmdb(it, pool, embyPool = emby.pool) }
        val covered = remoteEntries.map { TitleMatch.normalize(it.title) }.toSet()
        val nasOnly = local.nasEntries().filter { hit ->
            TitleMatch.matches(hit.title, q) || TitleMatch.matches(q, hit.title) ||
                hit.searchAlias?.let { TitleMatch.matches(it, q) } == true
        }.map { local.nasEntry(it) }
        val embyOnly = emby.search(q).map { emby.toEntry(it) }
        val localOnly = (nasOnly + embyOnly + pool.map { local.localEntry(it) })
            .filterNot { TitleMatch.normalize(it.title) in covered }
            .distinctBy { it.key }
        val all = localOnly + remoteEntries

        val err = if (all.isEmpty()) remote.errorKind?.let(::errorText) else null
        val notice = listOfNotNull(localState.denied, emby.notice)
            .joinToString("；").ifBlank { null }
        return Outcome(all, err, notice)
    }

    /** Emby 池就绪后强制重搜当前词（onQuery 同值早退，不能复用）。 */
    fun refresh() {
        val q = query
        if (q.isBlank()) return
        val ticket = ++generation
        job?.cancel()
        loading = true
        job = scope.launch {
            val outcome = withContext(Dispatchers.IO) { runSearch(q.trim()) }
            if (ticket != generation) return@launch
            entries = outcome.entries
            error = outcome.error
            localNotice = outcome.localNotice
            loading = false
        }
    }

    fun bestMatch(): SearchEntry? = entries.firstOrNull { it.playable } ?: entries.firstOrNull()

    fun restResults(): List<SearchEntry> {
        val best = bestMatch() ?: return emptyList()
        return entries.filterNot { it.key == best.key }
    }

    fun clear() {
        ++generation; job?.cancel()
        query = ""; entries = emptyList(); loading = false; error = null; localNotice = null
    }

    companion object {
        fun errorText(kind: String): String = when (kind) {
            "offline" -> "网络不可用，仅显示本地结果"
            "timeout" -> "连接超时，请稍后重试"
            else -> "远端搜索失败，请稍后重试"
        }
    }
}