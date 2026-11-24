package com.kivo.cinema.search

import android.content.Context
import androidx.compose.runtime.mutableStateOf
import com.kivo.cinema.emby.EmbyAuthCache
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyErrors
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.aggregate.AggregateSource
import com.kivo.cinema.library.TitleMatch
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.access.SourceEligibility
import com.kivo.cinema.sources.storage.SourceStore
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Semaphore
import kotlinx.coroutines.sync.withPermit
import kotlinx.coroutines.withContext
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Emby 渠道的可播索引（family-tree §二）：已授权源（非隐藏、开聚合搜索、非 SMB）→
 * 复用 `EmbyAuthCache` 登录 → 全库轻量标题池（Movie+Series，100/页，10 分钟内存缓存）。
 *
 * ② 搜索与 ③④ 可播匹配都读同一份内存池，**不为每次查询逐源发请求**。
 * 池未就绪时搜索先用 NAS/TMDB 出结果，就绪后由宿主触发重算；
 * 部分源失败必须留下可见原因（[notice]），不静默降级。
 */
class EmbyPlayableIndex(private val app: Context) {

    data class Entry(val source: AggregateSource, val item: EmbyItem)

    @Volatile var pool: List<Entry> = emptyList(); private set
    /** 索引中 / 部分源失败的可见原因；无 Emby 源属正常，不填。 */
    @Volatile var notice: String? = null; private set
    @Volatile var ready: Boolean = false; private set
    /** 池版本号：Compose 侧监听它触发重搜/重筛。 */
    val version = mutableStateOf(0)

    private var loadedAtMs = 0L
    private val running = AtomicBoolean(false)
    private val slots = Semaphore(2)

    fun refreshIfStale(scope: CoroutineScope) {
        if (running.get()) return
        if (ready && System.currentTimeMillis() - loadedAtMs < TTL_MS) return
        if (!running.compareAndSet(false, true)) return
        scope.launch {
            try {
                load()
            } finally {
                running.set(false)
            }
        }
    }

    private suspend fun load() {
        notice = null
        val records = withContext(Dispatchers.IO) {
            SourceRepository(SourceStore(app)).read()
        }.getOrElse {
            notice = "Emby 未接入：资源库读取失败"
            ready = true
            return
        }
        val eligible = SourceEligibility.search(records).filter {
            !it.isSmb && SourceEligibility.visible(it, SourceAccessSession.unlocked)
        }
        if (eligible.isEmpty()) {
            pool = emptyList(); ready = true; loadedAtMs = System.currentTimeMillis()
            version.value++
            return
        }
        notice = "Emby 正在索引，结果将陆续补全…"
        val failures = java.util.Collections.synchronizedList(mutableListOf<String>())
        val gathered = kotlinx.coroutines.coroutineScope {
            eligible.map { record ->
                async(Dispatchers.IO) {
                    slots.withPermit {
                        runCatching { loadSource(record) }.getOrElse {
                            failures += "${record.name}：${EmbyErrors.reason(it)}"
                            emptyList()
                        }
                    }
                }
            }.awaitAll()
        }
        pool = gathered.flatMap { it }
        ready = true
        loadedAtMs = System.currentTimeMillis()
        notice = if (failures.isEmpty()) null else "部分 Emby 源未接入：" + failures.joinToString("；")
        version.value++
    }

    private fun connect(record: SourceRecord): AggregateSource {
        EmbyAuthCache.get(record)?.let { (client, auth) ->
            return AggregateSource(record, client, auth)
        }
        val client = EmbyClient(record.address)
        val auth = client.login(record.username, record.password)
        EmbyAuthCache.remember(record, client, auth)
        return AggregateSource(record, client, auth)
    }

    /** 单源全库分页：sort=SortName 升序保证翻页稳定；上限 50 页（5000 条）防失控。 */
    private fun loadSource(record: SourceRecord): List<Entry> {
        val source = connect(record)
        val out = mutableListOf<Entry>()
        var start = 0
        repeat(MAX_PAGES) {
            val page = source.client.items(
                source.auth, start = start, limit = PAGE_SIZE,
                types = "Movie,Series", sort = 1, rich = true,
            )
            out += page.items.map { Entry(source, it) }
            start += page.items.size
            if (page.items.size < PAGE_SIZE) return out
        }
        return out
    }

    fun match(title: String, year: String): Entry? = pool.firstOrNull { entry ->
        yearCompatible(entry.item.year, year) &&
            (TitleMatch.matches(entry.item.name, title) || TitleMatch.matches(title, entry.item.name))
    }

    fun search(query: String): List<Entry> = pool.filter { entry ->
        TitleMatch.matches(entry.item.name, query) || TitleMatch.matches(query, entry.item.name)
    }

    fun toEntry(entry: Entry): SearchEntry = SearchEntry(
        key = "emby:${entry.source.id}:${entry.item.id}",
        title = entry.item.name,
        year = entry.item.year?.takeIf { it > 0 }?.toString() ?: "",
        rating = entry.item.rating?.toFloat() ?: 0f,
        genres = entry.item.genres,
        posterUrl = entry.item.posterUrl.ifBlank { null },
        backdropUrl = null,
        isTv = entry.item.series,
        playable = true,
        sourceLabel = "${entry.source.name} 可播",
        quality = null,
        emby = entry,
    )

    private companion object {
        const val TTL_MS = 10L * 60 * 1000
        const val PAGE_SIZE = 100
        const val MAX_PAGES = 50
    }
}