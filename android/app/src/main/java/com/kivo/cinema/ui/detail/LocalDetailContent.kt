package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.media.ThumbnailUtils
import android.provider.MediaStore
import com.kivo.cinema.evidence.IoTrace
import com.kivo.cinema.library.LocalVideoSource
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.player.resolutionTier
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * D12 · 本地文件详情内容域（第三来源，与 TMDB / Emby 并列喂同一个 `DetailPage`）。
 *
 * 与老 `localDetailContent` 的本质差异：**入口路径就是片源**。
 * - 老路径（TMDB → 标题反查本地文件）在多版本同名片源上会「点 A 播 B」；
 * - 这里 `playIntent` 固定用入口 path，TMDB 只做增强（海报/简介/演员），失败不影响播放。
 *
 * 铁律：本函数**永不返回 null**。本地文件存在却报「请检查网络」是旧实现的静默降级源头。
 * 本地详情**不查 TMDB**：文件即真相，断网/无 key 都秒开（旧页同样零网络依赖）；
 * 海报增强要等一次搜索，会把首帧拖到 8–15s 超时，不划算。
 */
internal class LocalFileFacts(val item: MediaItem, val path: String)

private const val THUMB_SCHEME = "kivo-thumb://"

/** 本地缩略图的自定义 URL：只作 [DetailContent.loadImage] 的键，不会被网络层解析。 */
internal fun localThumbUrl(path: String): String = THUMB_SCHEME + path

internal fun localFileContent(
    context: Context,
    path: String,
    durationHintMs: Long,
): DetailContent {
    val item = resolveLocalItem(context, path, durationHintMs)
    val title = fileTitle(item.title.ifBlank { path.substringAfterLast('/') })
    val details = fileOnlyDetails(item, title)
    val size = runCatching { java.io.File(item.path).length() }.getOrDefault(0L)
    val sec = (item.durationMs / 1000L).coerceAtLeast(1L)
    return DetailContent(
        details = details,
        media = ResourceMedia(
            resolution = if (item.width > 0 && item.height > 0) "${item.width} × ${item.height}" else "",
            sizeBytes = size,
            bitrateBps = if (size > 0L) size * 8L / sec else 0L,
            sourceLabel = "本地",
            streams = null,
        ),
        quality = resolutionTier(item.width, item.height),
        // 用入口 path，不走 MediaMatch：同名多版本时必须播用户点的那个文件。
        playIntent = Intent(context, PlayerComposeActivity::class.java)
            .putExtra("kivo.media", item.path)
            .putExtra("kivo.durationMs", item.durationMs),
        tmdbId = 0,
        tmdbIsTv = false,
        noPlayReason = "",
        serverName = "本地",
        loadImage = { url -> localThumbLoader(url) },
        local = LocalFileFacts(item, item.path),
    )
}

/** MediaStore 精确命中优先；推送进设备的测试片用传入字段兜底（不伪造宽高）。 */
internal fun resolveLocalItem(context: Context, path: String, durationHintMs: Long): MediaItem {
    val hit = runCatching {
        LocalVideoSource.query(context).firstOrNull { it.path == path }
    }.getOrNull()
    if (hit != null) return hit
    return MediaItem(
        id = 0, title = path.substringAfterLast('/'), path = path,
        durationMs = durationHintMs, width = 0, height = 0, addedSec = 0,
    )
}

/** 无 TMDB key / 断网 / 未匹配：用文件事实拼一个最小详情体，页面照常可读可播。 */
private fun fileOnlyDetails(item: MediaItem, title: String): TmdbDetail.Details = TmdbDetail.Details(
    id = 0,
    title = title,
    date = "",
    genres = emptyList(),
    overview = "",
    rating = 0f,
    posterUrl = "",
    backdropUrl = null,
    heroArtworkUrl = localThumbUrl(item.path),
    logoUrl = null,
    isTv = false,
    region = null,
    episodes = null,
    // 不足 1 分钟（或 MediaStore 没解析出时长）不画片长，避免 meta 行出现"0分钟"。
    runtimeMin = if (item.durationMs >= 60_000L) (item.durationMs / 60_000L).toInt() else null,
    trailerKey = null,
    cast = emptyList(),
    stills = emptyList(),
    studioLogo = null,
    certifications = emptyList(),
    contentNotes = emptyList(),
    similar = emptyList(),
)

/** 本地文件首帧作 hero；非本 scheme 的 URL 交回默认加载器（TMDB 图仍走网络缓存）。 */
private suspend fun localThumbLoader(url: String): Bitmap? {
    if (!url.startsWith(THUMB_SCHEME)) return null
    val path = url.removePrefix(THUMB_SCHEME)
    return withContext(Dispatchers.IO) {
        IoTrace.measure("detail.localHero") {
            runCatching {
                @Suppress("DEPRECATION")
                ThumbnailUtils.createVideoThumbnail(path, MediaStore.Video.Thumbnails.MINI_KIND)
            }.getOrNull()
        }
    }
}

private fun fileTitle(raw: String): String {
    val name = raw.substringAfterLast('/')
    val dot = name.lastIndexOf('.')
    val base = if (dot > 0) name.substring(0, dot) else name
    return base.ifBlank { "本地视频" }
}
