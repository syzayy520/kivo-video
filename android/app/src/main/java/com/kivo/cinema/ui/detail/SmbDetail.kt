package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.MediaTitleParser
import com.kivo.cinema.library.NasIndexStore
import com.kivo.cinema.library.NasLibraryGrouping
import com.kivo.cinema.library.NasVideo
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.design.home.sections.resolvePoster
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * NAS(SMB) 片源 → 详情内容（第四个来源，与 TMDB / 本地文件 / Emby 并列喂同一个 `DetailPage`）。
 *
 * 【为什么需要它】首页片库卡片以前点了**直接起播**，用户看不到简介/评分/选集。
 * 现在统一走详情页，和 Emby/本地片源的交互一致。
 *
 * 【两种形态】
 * - 单片：[smbSingleDetail] —— 一个文件即一部片；
 * - 剧集：[smbSeriesDetail] —— 同目录多集聚合，复用 `DetailPage` 既有的季集节（[EmbyShows.Season]/[Episode]
 *   是通用数据类，不是 Emby 专属）。
 *
 * 片库索引（[NasIndexStore]）是唯一数据源：**不在这里扫 SMB 目录**（那是分钟级操作）。
 * 索引读不到时如实降级为"只能看、不能播"，不伪造可播。
 */
internal object SmbDetail {

    /** 单片：一个文件 → 详情。 */
    fun single(context: Context, sourceId: String, share: String, path: String): DetailContent {
        val fileName = path.substringAfterLast('/')
        val parsed = MediaTitleParser.parse(fileName)
        val video = findVideo(context, sourceId, share, path)
        val facts = onlyDetails(parsed.title.ifBlank { fileName }, parsed.year)
        // 限时增强：拿得到就补简介/评分/演员；拿不到（无 key/无网/超时）仍是可读可播的文件事实页。
        val details = (enrich(context, parsed.title, parsed.year, preferTv = false) ?: facts)
            .withLocalHero()
        // 码率用「文件体积 ÷ TMDB 时长」估算：能算就填，算不出（缺时长）给 0 让 UI 不显示。
        val runSec = ((details.runtimeMin ?: 0) * 60).toLong()
        val bps = video?.size?.takeIf { it > 0L && runSec > 0L }?.let { it * 8L / runSec } ?: 0L
        return DetailContent(
            details = details,
            // 片源就是入口那个文件：给出真实体积，别让页面显示"暂无可用资源"（文件明明在）。
            media = video?.let {
                ResourceMedia(
                    resolution = "",   // 不探测分辨率：要读远端文件头，不值得为一行胶囊付网络
                    sizeBytes = it.size,
                    bitrateBps = bps,
                    sourceLabel = "NAS",
                    streams = null,
                )
            },
            quality = "",
            playIntent = playIntent(context, sourceId, share, path, fileName),
            tmdbId = 0,
            tmdbIsTv = false,
            noPlayReason = "",
            serverName = "NAS",
            // 幕布专用：只有 [NAS_HERO_SCHEME] 才走本地海报（并裁掉底部烧死的片名）。
            // 其余 URL（演员头像/剧照/相似海报）**必须返回 null**，交给各自的默认加载器 ——
            // 否则所有头像都会变成同一张图（真机上就是这么出错的）。
            loadImage = { url ->
                if (!url.startsWith(NAS_HERO_SCHEME)) null
                else loadPoster(context, sourceId, share, path, video?.posterPath, parsed.title, parsed.year)
                    ?.trimTitleBand()
            },
        )
    }

    /**
     * 剧集：按 `(父目录, 剧名)` 定位一部剧（可能含多季）→ 详情 + 选集。
     * 索引里找不到时退化为单片（仍可播放第 1 集）。
     */
    fun series(
        context: Context,
        sourceId: String,
        share: String,
        rootDir: String,
        title: String,
    ): DetailContent {
        val candidates = runCatching { NasIndexStore(context).read() }.getOrNull().orEmpty()
            .filter {
                it.sourceId == sourceId && it.share == share &&
                    it.dirPath.substringBeforeLast('/', "") == rootDir
            }
        val allSeries = NasLibraryGrouping.group(candidates)
            .filterIsInstance<NasLibraryGrouping.Item.Series>()
        val series = allSeries.firstOrNull { it.series.title == title }?.series
            ?: allSeries.firstOrNull()?.series
        if (series == null) {
            // 索引过期/刚改过文件：按"目录名当标题"给一个可读页面，但播放交给第 1 个已知文件。
            val fallback = candidates.minByOrNull { it.path }
                ?: return unavailable(context, sourceId, share, rootDir.ifBlank { title })
            return single(context, sourceId, share, fallback.path)
        }
        val seasons = series.seasons.map { s ->
            EmbyShows.Season("nas-s${s.index}", "第 ${s.index} 季", s.index)
        }
        val firstSeason = series.seasons.firstOrNull()?.episodes.orEmpty()
        val first = series.firstEpisode
        // 剧集走 playEpisode（DetailPage 在 loadEpisodes != null 时忽略 playIntent 的默认跳转）。
        val defaultIntent = first.path.takeIf { it.isNotBlank() }
            ?.let { playIntent(context, sourceId, share, it, first.label) }
        val facts = onlyDetails(series.title, series.year)
        val details = (enrich(context, series.title, series.year, preferTv = true) ?: facts)
            .withLocalHero()
        return DetailContent(
            details = details,
            media = null,
            quality = "",
            playIntent = defaultIntent,
            tmdbId = 0,
            tmdbIsTv = true,
            noPlayReason = "",
            seasons = seasons,
            episodes = firstSeason.map { toEpisode(it) },
            loadEpisodes = { season ->
                series.seasons.firstOrNull { it.index == season.index }?.episodes
                    ?.map { toEpisode(it) }.orEmpty()
            },
            playEpisode = { ep -> playIntent(context, sourceId, share, ep.id, ep.name) },
            serverName = "NAS",
            loadImage = { url ->
                if (!url.startsWith(NAS_HERO_SCHEME)) null
                else loadPoster(context, sourceId, share, first.path, series.posterPath, series.title, series.year)
                    ?.trimTitleBand()
            },
        )
    }

    /** 幕布专用 scheme：只用于让 [loadImage] 认出"这一张该用本地海报"。 */
    private const val NAS_HERO_SCHEME = "kivo-nas-hero://"

    /**
     * 把幕布地址改成 [NAS_HERO_SCHEME]。
     *
     * 【为什么要改】`loadImage` 是**一个**回调，既服务幕布（页级大图）又服务演员头像/剧照/相似海报。
     * 只要它"忽略参数一律返回本地海报"，那些小图就全变成同一张 —— 真机表现为所有演员同一张头像、剧照全灰。
     * 给幕布一个专属 scheme 后，[loadImage] 只认它，其它 URL 返回 null 交回默认加载器，各取各图。
     */
    private fun TmdbDetail.Details.withLocalHero(): TmdbDetail.Details =
        copy(heroArtworkUrl = NAS_HERO_SCHEME + "hero")

    /** 增强预算：超过就用文件事实（详情页不能被网络拖住）。 */
    private const val TMDB_BUDGET_MS = 2_500L

    /**
     * 用 TMDB 增强（简介/评分/类型/演员/相似）：**限时 2.5s**，超时即用文件事实。
     *
     * 【为什么限时】`TmdbClient` 的 connect/read 超时是 8s/15s，若源站慢，详情页会卡十几秒才出内容。
     * 详情页已有 preview 快照先显示标题，所以这里宁可放弃增强也不能拖住首屏（D376 教训：Future.get 必带超时）。
     * 两次请求（search → details）都有 12h 磁盘缓存，二次进秒回。
     */
    private fun enrich(context: Context, title: String, year: Int?, preferTv: Boolean): TmdbDetail.Details? {
        if (title.isBlank()) return null
        val pool = java.util.concurrent.Executors.newSingleThreadExecutor()
        val task = java.util.concurrent.Callable {
            runCatching {
                val client = TmdbClient(context.applicationContext)
                if (client.apiKey().isEmpty()) return@runCatching null
                val detail = TmdbDetail(client)
                // 先按首选类型搜，搜不到再换另一种（NAS 分类不一定准，两种都试一次）。
                listOf(preferTv, !preferTv).firstNotNullOfOrNull { asTv ->
                    client.searchId(title, year, asTv)?.let { id ->
                        detail.details(asTv, id)?.also { d ->
                            com.kivo.cinema.net.NetDiag.log(
                                "[smb-detail] tmdb ok id=$id cast=${d.cast.size} genres=${d.genres.size} " +
                                    "stills=${d.stills.size} similar=${d.similar.size} logo=${d.logoUrl != null} " +
                                    "studio=${d.studioLogo != null} certs=${d.certifications.size}",
                            )
                        }
                    }
                }
            }.getOrNull()
        }
        return try {
            pool.submit(task).get(TMDB_BUDGET_MS, java.util.concurrent.TimeUnit.MILLISECONDS)
        } catch (e: Exception) {
            com.kivo.cinema.net.NetDiag.log("[smb-detail] tmdb 增强放弃 ${e::class.java.simpleName}")
            null
        } finally {
            pool.shutdownNow()
        }
    }

    /** 索引不可用：页面能看标题，但明确告知不能播（不假装可播再报错）。 */
    private fun unavailable(context: Context, sourceId: String, share: String, dirPath: String): DetailContent {
        val title = MediaTitleParser.parse(dirPath.substringAfterLast('/').ifBlank { dirPath }).title
        return DetailContent(
            details = onlyDetails(title, null),
            media = null,
            quality = "",
            playIntent = null,
            tmdbId = 0,
            tmdbIsTv = true,
            noPlayReason = "片库索引已过期，请回首页下拉刷新后重试",
            serverName = "NAS",
        )
    }

    private fun toEpisode(e: com.kivo.cinema.library.NasEpisode): EmbyShows.Episode =
        EmbyShows.Episode(
            id = e.path,
            name = e.label.ifBlank { e.fileName },
            index = if (e.index > 0) e.index else 1,
            seasonIndex = e.seasonIndex,
            runtimeMinutes = 0,
            thumbUrl = "",
            played = false,
            positionTicks = 0,
            airDate = "",
            overview = "",
        )

    private fun playIntent(
        context: Context,
        sourceId: String,
        share: String,
        path: String,
        label: String,
    ): Intent = Intent(context, PlayerComposeActivity::class.java)
        .putExtra("kivo.sourceId", sourceId)
        .putExtra("kivo.smbShare", share)
        .putExtra("kivo.smbPath", path)
        .putExtra("kivo.label", label)

    private fun findVideo(context: Context, sourceId: String, share: String, path: String): NasVideo? =
        runCatching { NasIndexStore(context).read() }.getOrNull()
            ?.firstOrNull { it.sourceId == sourceId && it.share == share && it.path == path }

    /** 海报：与首页卡片同一套（库内图 → TMDB → 空），保证首页有图时详情页不会反而空。 */
    private suspend fun loadPoster(
        context: Context,
        sourceId: String,
        share: String,
        path: String,
        posterPath: String?,
        title: String,
        year: Int?,
    ): Bitmap? = withContext(Dispatchers.IO) {
        resolvePoster(
            context,
            HomeEntry(
                key = "smb://$sourceId/$share/$path",
                title = title,
                sourceLabel = "NAS",
                addedSec = 0L,
                positionMs = 0L,
                durationMs = 0L,
                kind = HomeEntry.Kind.Smb,
                smbPosterPath = posterPath,
                year = year,
            ),
        )
    }

    /**
     * 裁掉海报底部 15%：那里通常烧着片名，会和 TitleLayer 的片名/logo 叠成重影。
     *
     * 只裁底部、不缩放，尽量不破坏人物构图；裁失败（OOM/异常）就用原图，宁可重影也不能空着。
     */
    private fun Bitmap.trimTitleBand(): Bitmap {
        val keep = (height * 0.85f).toInt().coerceAtLeast(1)
        if (keep >= height) return this
        return runCatching { Bitmap.createBitmap(this, 0, 0, width, keep) }.getOrDefault(this)
    }

    /** 文件事实最小详情：没有 TMDB 匹配也照常可读（不显示伪造的评分/简介）。 */
    private fun onlyDetails(title: String, year: Int?): TmdbDetail.Details = TmdbDetail.Details(
        id = 0,
        title = title,
        date = year?.toString() ?: "",
        genres = emptyList(),
        overview = "",
        rating = 0f,
        posterUrl = "",
        backdropUrl = null,
        heroArtworkUrl = "",
        logoUrl = null,
        isTv = false,
        region = null,
        episodes = null,
        runtimeMin = null,
        trailerKey = null,
        cast = emptyList(),
        stills = emptyList(),
        studioLogo = null,
        certifications = emptyList(),
        contentNotes = emptyList(),
        similar = emptyList(),
    )

}
