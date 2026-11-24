package com.kivo.cinema.search

import android.content.Context
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.library.MediaTitleParser
import com.kivo.cinema.library.NasCategory
import com.kivo.cinema.library.NasIndexStore
import com.kivo.cinema.library.NasLibrarySource
import com.kivo.cinema.library.TitleMatch
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore

/**
 * 本地可播判定（family-tree §二「可播判定」）：
 * NAS 索引（过滤已删除/隐藏源）+ 本次 MediaStore 命中集合，按「标题归一化 + 年份相容」匹配。
 *
 * 单一职责：回答「这个标题在本地有没有片、是哪一条」，并产出本地 [SearchEntry]。
 * 不联网、不做 UI；索引读取阻塞，须在 IO 线程调用。
 */
class LocalPlayableIndex(private val app: Context) {

    @Volatile private var nasCache: List<HomeEntry>? = null

    /** NAS 全量条目（六个分类合并，不按首页 12 条截断）。索引缺失 = 空表，不是错误。 */
    fun nasEntries(): List<HomeEntry> {
        nasCache?.let { return it }
        val alive = runCatching {
            SourceRepository(SourceStore(app)).read().getOrNull()
                ?.filter { !it.hidden }?.map { it.id }?.toSet()
        }.getOrNull()
        val videos = NasIndexStore(app).read().filter { alive == null || it.sourceId in alive }
        val all = NasCategory.ordered.flatMap { category ->
            NasLibrarySource.entries(videos, category, limit = Int.MAX_VALUE)
        }
        nasCache = all
        return all
    }

    /** 索引被后台重扫后调用，下一次搜索读新索引。 */
    fun invalidate() { nasCache = null }


    fun matchNas(title: String, year: String): HomeEntry? = nasEntries().firstOrNull { entry ->
        yearCompatible(entry.year, year) &&
            (TitleMatch.matches(entry.title, title) || TitleMatch.matches(title, entry.title) ||
                entry.searchAlias?.let { TitleMatch.matches(it, title) } == true)
    }

    fun matchLocal(title: String, year: String, pool: List<MediaItem>): MediaItem? =
        pool.firstOrNull { item ->
            val parsed = MediaTitleParser.parse(item.title)
            yearCompatible(parsed.year, year) &&
                (TitleMatch.matches(item.title, title) || TitleMatch.matches(title, parsed.title))
        }

    /** NAS 命中 → 可播条目（海报由 UI 侧走既有 entryPoster/NasPoster 管线时另带；这里给 TMDB 兜底 URL）。 */
    fun nasEntry(home: HomeEntry): SearchEntry = SearchEntry(
        key = "nas:" + home.key,
        title = home.title,
        year = home.year?.takeIf { it > 0 }?.toString() ?: "",
        rating = 0f,
        genres = emptyList(),
        posterUrl = home.posterUrl,
        backdropUrl = null,
        isTv = home.episodeLabel != null || home.key.startsWith("smb-series://"),
        playable = true,
        sourceLabel = "NAS 可播",
        quality = qualityBadgeOf(home.key, 0),
        homeEntry = home,
    )

    /** TMDB 条目 → 统一结果：逐条做本地可播匹配（NAS → Emby → 本机命中池）。 */
    fun fromTmdb(
        item: com.kivo.cinema.tmdb.TmdbHome.Item,
        pool: List<MediaItem>,
        embyPool: List<EmbyPlayableIndex.Entry> = emptyList(),
        emby: EmbyPlayableIndex? = null,
    ): SearchEntry {
        val nas = matchNas(item.title, item.year())
        val embyHit = if (nas == null) emby?.match(item.title, item.year())
            ?: embyPool.firstOrNull { entry ->
                yearCompatible(entry.item.year, item.year()) &&
                    (TitleMatch.matches(entry.item.name, item.title) ||
                        TitleMatch.matches(item.title, entry.item.name))
            } else null
        val media = if (nas == null && embyHit == null) matchLocal(item.title, item.year(), pool) else null
        return SearchEntry(
            key = "tmdb:${item.isTv}:${item.id}",
            title = item.title,
            year = item.year(),
            rating = item.rating,
            genres = item.genres,
            posterUrl = item.posterUrl,
            backdropUrl = item.backdropUrl,
            isTv = item.isTv,
            playable = nas != null || embyHit != null || media != null,
            sourceLabel = when {
                nas != null -> "NAS 可播"
                embyHit != null -> runCatching {
                    com.kivo.cinema.emby.EmbyServerNames.resolve(
                        app, embyHit.source.client, embyHit.source.id, embyHit.source.name,
                    )
                }.getOrDefault(embyHit.source.name) + " 可播"
                media != null -> "本地可播"
                else -> "TMDB · 未入库"
            },
            quality = nas?.let { qualityBadgeOf(it.key, 0) }
                ?: media?.let { qualityBadgeOf(it.title, it.width) },
            tmdb = item,
            homeEntry = nas,
            mediaItem = media,
            emby = embyHit,
        )
    }

    fun localEntry(item: MediaItem): SearchEntry {
        val parsed = MediaTitleParser.parse(item.title)
        return SearchEntry(
            key = "local:" + item.path,
            title = parsed.title.ifBlank { item.title },
            year = parsed.year?.takeIf { it > 0 }?.toString() ?: "",
            rating = 0f,
            genres = emptyList(),
            posterUrl = null,
            backdropUrl = null,
            isTv = parsed.episode != null,
            playable = true,
            sourceLabel = "本地可播",
            quality = qualityBadgeOf(item.title, item.width),
            mediaItem = item,
        )
    }
}