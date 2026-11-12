package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.widget.Toast
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyDetails
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.media.marks.FilmKey
import com.kivo.cinema.media.marks.FilmMark
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.player.resolutionTier
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.decodeImageCapped
import com.kivo.cinema.ui.detail.actions.DetailActions
import com.kivo.cinema.ui.detail.actions.rememberDetailActions
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Emby 详情桥接（254 D14）：Emby 真值 + TMDB 智能填充组装成 [DetailContent]。
 * 收藏/已看覆盖为服务端事实（乐观更新 + 失败回滚 + 非空 reason），待看仍走本地真实标记。
 * 不新造详情节组件——资源槽复用 DetailResource 同一套卡，编排复用 DetailPage。
 */
internal class EmbyResource private constructor(
    private val context: Context,
    private val sourceId: String?,
    private val itemId: String,
    private val token: String?,
    private val userId: String?,
    private val name: String,
    private val year: Int?,
    private val serverName: String,
    private val series: Boolean,
) {
    private var serverFavorite by mutableStateOf(false)
    private var serverWatched by mutableStateOf(false)
    private var tmdbId by mutableStateOf(0)
    private var serverError by mutableStateOf<String?>(null)
    private var saving by mutableStateOf(false)
    private var client: EmbyClient? = null
    private var auth: EmbyClient.Auth? = null

    /** IO：Emby 真值 + TMDB 填充（命中）/ 文本兜底（无 key/无匹配）。失败返回 null 由页面呈现。 */
    fun loadDetails(): DetailContent? {
        if (!ensureClient()) return null
        val c = client!!; val a = auth!!
        val emby = c.details(a, itemId)
        serverFavorite = emby.isFavorite
        serverWatched = emby.played
        val tmdb = TmdbClient(context)
        val hit = tmdb.searchId(emby.name.ifBlank { name }, emby.year ?: year, series)
        val filled = if (hit != null) {
            tmdbId = hit
            TmdbDetail(tmdb).details(series, hit) ?: EmbyFallback.map(emby, series)
        } else {
            tmdbId = 0
            EmbyFallback.map(emby, series)
        }
        val details = qualityFirst(filled, emby, c, a)
        if (series) return seriesContent(c, a, details)
        val media = emby.media?.let {
            val res = if (it.width > 0 && it.height > 0) "${it.width} × ${it.height}" else "媒体信息未提供"
            ResourceMedia(res, it.sizeBytes, it.bitrate, serverName,
                ResourceStreams(it.container, it.video, it.audio, it.subtitles))
        }
        val quality = emby.media?.takeIf { it.width > 0 }
            ?.let { resolutionTier(it.width, it.height).ifEmpty { "SD" } } ?: "未知"
        return DetailContent(details, media, quality, playIntent(c, a), tmdbId, false,
            loadImage = ::loadImage, serverName = serverName)
    }

    /** 255 #30 图择优：海报/幕布按像素宽选更清者（同级库优先，与海报墙同源）；logo 走中文规则。 */
    private fun qualityFirst(d: TmdbDetail.Details, e: EmbyDetails,
                             c: EmbyClient, a: EmbyClient.Auth): TmdbDetail.Details {
        val w = runCatching { c.imageWidths(a, itemId) }.getOrDefault(emptyMap())
        val poster = pickHigher(e.posterUrl, w["Primary"] ?: 0, d.posterUrl, d.posterWidth)
        val backdrop = pickHigher(e.backdropUrl, w["Backdrop"] ?: 0, d.backdropUrl.orEmpty(), d.backdropWidth)
        val libLogo = e.logoUrl.takeIf { it.isNotBlank() }
        // 中文原产剧优先库 logo（服务器 curated，如谷雨书法字）；否则 TMDB 中文 logo > 最佳 logo > 库 logo。
        val logo = if (d.originalLanguage == "zh" && libLogo != null) libLogo
            else d.zhLogoUrl ?: d.logoUrl ?: libLogo
        return d.copy(
            posterUrl = poster,
            backdropUrl = backdrop.ifBlank { null },
            heroArtworkUrl = backdrop.ifBlank { poster }.ifBlank { d.heroArtworkUrl },
            logoUrl = logo,
        )
    }

    /** 像素宽更大者胜；相等或一方缺图时库优先。 */
    private fun pickHigher(libUrl: String, libW: Int, tmdbUrl: String, tmdbW: Int): String = when {
        libUrl.isBlank() -> tmdbUrl
        tmdbUrl.isBlank() -> libUrl
        tmdbW > libW -> tmdbUrl
        else -> libUrl
    }

    /** 剧集（255）：季 + 首季集预载；播放钮 = 首集；图走 token 加载。
     *  季/集拉取失败直接抛出 → 页面错误态 + 重试（不静默兜底）。 */
    private fun seriesContent(c: EmbyClient, a: EmbyClient.Auth, details: TmdbDetail.Details): DetailContent {
        val shows = EmbyShows(c)
        val seasons = shows.seasons(a, itemId)
        val episodes = seasons.firstOrNull()
            ?.let { stillsFirst(shows.episodes(a, itemId, it.id), it.index) } ?: emptyList()
        return DetailContent(
            details = details, media = null, quality = "",
            playIntent = episodes.firstOrNull()?.let { episodeIntent(c, a, it) },
            tmdbId = tmdbId, tmdbIsTv = true,
            noPlayReason = "该服务器未返回可播放的剧集",
            seasons = seasons, episodes = episodes,
            loadEpisodes = { s -> stillsFirst(shows.episodes(a, itemId, s.id), s.index) },
            playEpisode = { ep -> episodeIntent(c, a, ep) },
            loadImage = ::loadImage,
            serverName = serverName,
        )
    }

    /** 集缩略图优先级：库图优先；服务器缺图才 TMDB 剧照兜底（与海报同口径）。 */
    private fun stillsFirst(eps: List<EmbyShows.Episode>, seasonIndex: Int): List<EmbyShows.Episode> {
        if (tmdbId == 0 || eps.none { it.thumbUrl.isBlank() }) return eps
        val stills = runCatching { TmdbClient(context).seasonStills(tmdbId, seasonIndex) }
            .getOrDefault(emptyMap())
        if (stills.isEmpty()) return eps
        return eps.map { ep ->
            if (ep.thumbUrl.isNotBlank()) ep
            else stills[ep.index]?.let { ep.copy(thumbUrl = it) } ?: ep
        }
    }

    private fun episodeIntent(c: EmbyClient, a: EmbyClient.Auth, ep: EmbyShows.Episode): Intent =
        Intent(context, PlayerComposeActivity::class.java)
            .putExtra("kivo.playurl", c.streamUrl(a, ep.id))
            .putExtra("kivo.label", "$name · " + "S%02dE%02d".format(ep.seasonIndex, ep.index) + " " + ep.name)
            .putExtra("kivo.sourceName", serverName)
            .putExtra("kivo.sourceId", sourceId)
            .putExtra("kivo.embyToken", a.token)

    /** Emby 图需 token（TmdbImageCache 不带）：按域路由——本服务器头鉴权，外域（TMDB）不带 token。 */
    private suspend fun loadImage(url: String): Bitmap? = withContext(Dispatchers.IO) {
        val c = client ?: return@withContext null
        if (url.isBlank()) return@withContext null
        runCatching {
            if (url.startsWith(c.base)) {
                val bytes = c.fetchBytes(url, token)
                decodeImageCapped(bytes)
            } else {
                TmdbImageCache.load(url)
            }
        }.getOrNull()
    }

    /** 收藏/已看 = 服务端真值覆盖；待看 = 本地。toggle 拦截 FAVORITE/WATCHED 走 Emby。 */
    @Composable
    fun actions(): DetailActions {
        val key = remember(tmdbId, itemId) {
            FilmKey(if (tmdbId > 0) tmdbId else (itemId.hashCode() and 0x7fffffff).coerceAtLeast(1), false)
        }
        val local = rememberDetailActions(key)
        val scope = rememberCoroutineScope()
        return DetailActions(
            marks = local.marks.copy(favorite = serverFavorite, watched = serverWatched),
            enabled = local.enabled && !saving,
            error = serverError ?: local.error,
            toggle = { mark ->
                when (mark) {
                    FilmMark.FAVORITE, FilmMark.WATCHED -> serverToggle(mark, scope)
                    FilmMark.WATCHLIST -> local.toggle(mark)
                }
            },
            retry = { serverError = null; local.retry() },
        )
    }

    private fun serverToggle(mark: FilmMark, scope: CoroutineScope) {
        if (saving) return
        val c = client ?: return; val a = auth ?: return
        val on = if (mark == FilmMark.FAVORITE) !serverFavorite else !serverWatched
        val prevFav = serverFavorite; val prevWatched = serverWatched
        if (mark == FilmMark.FAVORITE) serverFavorite = on else serverWatched = on
        saving = true; serverError = null
        scope.launch {
            val ok = withContext(Dispatchers.IO) {
                runCatching {
                    if (mark == FilmMark.FAVORITE) c.setFavorite(a, itemId, on) else c.setPlayed(a, itemId, on)
                }.getOrDefault(false)
            }
            saving = false
            if (ok) Toast.makeText(context, markLabel(mark, on), Toast.LENGTH_SHORT).show()
            else {
                serverFavorite = prevFav; serverWatched = prevWatched
                serverError = "服务器未接受此标记，请检查网络后重试"
            }
        }
    }

    private fun playIntent(c: EmbyClient, a: EmbyClient.Auth): Intent =
        Intent(context, PlayerComposeActivity::class.java)
            .putExtra("kivo.playurl", c.streamUrl(a, itemId))
            .putExtra("kivo.label", name)
            .putExtra("kivo.sourceName", serverName)
            .putExtra("kivo.sourceId", sourceId)
            .putExtra("kivo.embyToken", a.token)

    private fun ensureClient(): Boolean {
        if (client != null && auth != null) return true
        val t = token; val u = userId
        if (t.isNullOrBlank() || u.isNullOrBlank()) return false
        val address = runCatching {
            SourceRepository(SourceStore(context)).read().getOrThrow()
                .firstOrNull { it.id == sourceId }?.address
        }.getOrNull() ?: return false
        return runCatching {
            client = EmbyClient(address)
            auth = EmbyClient.Auth(t, u)
        }.isSuccess
    }

    private fun markLabel(mark: FilmMark, on: Boolean): String = when (mark) {
        FilmMark.FAVORITE -> if (on) "已收藏" else "已取消收藏"
        FilmMark.WATCHED -> if (on) "已标记为已观看" else "已取消已观看标记"
        FilmMark.WATCHLIST -> if (on) "已添加到待看" else "已移出待看"
    }

    companion object {
        fun from(context: Context, intent: Intent, itemId: String): EmbyResource = EmbyResource(
            context = context.applicationContext,
            sourceId = intent.getStringExtra(MovieDetailActivity.EXTRA_EMBY_SOURCE),
            itemId = itemId,
            token = intent.getStringExtra(MovieDetailActivity.EXTRA_EMBY_TOKEN),
            userId = intent.getStringExtra(MovieDetailActivity.EXTRA_EMBY_USER),
            name = intent.getStringExtra(MovieDetailActivity.EXTRA_EMBY_NAME).orEmpty(),
            year = intent.getIntExtra(MovieDetailActivity.EXTRA_EMBY_YEAR, 0).takeIf { it > 0 },
            serverName = intent.getStringExtra(MovieDetailActivity.EXTRA_EMBY_SERVER).orEmpty().ifBlank { "Emby" },
            series = intent.getBooleanExtra(MovieDetailActivity.EXTRA_EMBY_SERIES, false),
        )
    }
}