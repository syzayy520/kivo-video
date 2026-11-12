package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.library.LocalVideoSource
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/**
 * 详情内容数据域（255 从 DetailPage 抽出）：本地 TMDB 与 Emby 各产出一份 [DetailContent]
 * 喂同一套编排；差异（资源槽/播放 intent/季集/图加载）都在内容里解析，页面本身无来源分支。
 * [loadEpisodes] 非空 = 剧集来源（季集目前只有 Emby 提供）；季集数据直接复用
 * [EmbyShows.Season]/[EmbyShows.Episode]，不另造平行数据类。
 */
internal class DetailContent(
    val details: TmdbDetail.Details,
    val media: ResourceMedia?,     // null → 空态块（本地无匹配）；剧集 → 季集节
    val quality: String,           // 播放资源胶囊（4K/1080P/…）
    val playIntent: Intent?,       // null → 播放按钮提示 noPlayReason
    val tmdbId: Int,               // >0 → TMDB 外链；0 → 诚实 Toast
    val tmdbIsTv: Boolean,
    val noPlayReason: String = DETAIL_NO_LOCAL_PLAY,
    val seasons: List<EmbyShows.Season> = emptyList(),
    val episodes: List<EmbyShows.Episode> = emptyList(),   // 首季集（预载，免首屏转圈）
    val loadEpisodes: (suspend (EmbyShows.Season) -> List<EmbyShows.Episode>)? = null,
    val playEpisode: ((EmbyShows.Episode) -> Intent)? = null,
    val loadImage: (suspend (String) -> Bitmap?)? = null,  // Emby 图需 token，TmdbImageCache 不带
)

internal const val DETAIL_NO_LOCAL_PLAY = "没有匹配的本地视频，请先在资源库添加资源"

/** 本地来源：TMDB 详情 + 本地片源匹配（240 §9.2）。无匹配 → media/playIntent 留空走空态。 */
internal fun localDetailContent(context: Context, tmdb: TmdbDetail, isTv: Boolean, id: Int): DetailContent? {
    val d = tmdb.details(isTv, id) ?: return null
    val mm = MediaMatch.find(LocalVideoSource.query(context), d.title)
    return DetailContent(
        details = d,
        media = mm?.let { ResourceMedia(it.resolution, it.sizeBytes, it.bitrateBps, "本地", null) },
        quality = mm?.quality ?: "",
        playIntent = mm?.let {
            Intent(context, PlayerComposeActivity::class.java)
                .putExtra("kivo.media", it.item.path)
                .putExtra("kivo.durationMs", it.item.durationMs)
        },
        tmdbId = id, tmdbIsTv = isTv,
    )
}