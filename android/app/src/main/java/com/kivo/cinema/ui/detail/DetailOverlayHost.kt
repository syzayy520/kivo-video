package com.kivo.cinema.ui.detail

import android.content.Intent
import android.widget.Toast
import androidx.activity.compose.BackHandler
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.media.marks.FilmMark
import com.kivo.cinema.ui.detail.actions.DetailActions
import com.kivo.cinema.ui.detail.actions.DetailMorePopup
import com.kivo.cinema.ui.detail.episodes.EpisodeGridSheet
import com.kivo.cinema.ui.detail.resource.ResourceVersionSheet
import com.kivo.cinema.ui.detail.version.VersionSourceFailure
import com.kivo.cinema.ui.subtitles.online.OnlineSubtitleSheetContent
import com.kivo.cinema.ui.subtitles.online.OnlineSubtitleSheetFrame

/** 255 #31 浮层状态：更多菜单锚点/开合 + 日历开合。DetailPage 只持有一个控制器。 */
internal class DetailOverlayState {
    var moreOpen by mutableStateOf(false)
    var moreAnchor by mutableStateOf(Rect.Zero)
    var calendarOpen by mutableStateOf(false)
    var episodeGridOpen by mutableStateOf(false)
    var versionOpen by mutableStateOf(false)
    /** 在线字幕面板（字幕准备移到播放之外：详情页搜索/下载/翻译，design.md §八）。 */
    var onlineSubtitleOpen by mutableStateOf(false)
    fun openMore(anchor: Rect) { moreAnchor = anchor; moreOpen = true }
}

/**
 * 「更多」扁平菜单 + 追剧日历浮层的编排宿主（从 DetailPage 抽出，控行数）。
 * 添加到日历＝真待看标记 + 开日历；追剧进度＝开日历；下载＝尚未开放，如实提示（不假装下载）。
 * 分享＝真 ACTION_SEND；移除＝真待看开关（仅在待看时显示）。
 */
@Composable
internal fun DetailOverlayHost(
    overlay: DetailOverlayState,
    content: DetailContent?,
    season: EmbyShows.Season?,
    episodes: List<EmbyShows.Episode>,
    actions: DetailActions,
    openPage: (Intent) -> Unit,
    onSelectEpisode: (EmbyShows.Episode) -> Unit = {},
    versions: List<ResourceVersion> = emptyList(),
    selectedVersionId: String? = null,
    versionsLoading: Boolean = false,
    versionFailures: List<VersionSourceFailure> = emptyList(),
    onSelectVersion: (ResourceVersion) -> Unit = {},
    menuOpen: Boolean = false,
    onMenuClose: () -> Unit = {},
) {
    val context = LocalContext.current
    val d = content?.details
    var confirmRemove by remember { mutableStateOf(false) }
    DetailMorePopup(overlay.moreOpen, overlay.moreAnchor,
        onDismiss = { overlay.moreOpen = false },
        onAddCalendar = {
            overlay.moreOpen = false
            if (!actions.marks.watchlist) actions.toggle(FilmMark.WATCHLIST)
            overlay.calendarOpen = true
        },
        onDownload = {
            overlay.moreOpen = false
            // 「下载」= 在线字幕（搜索/下载/翻译，字幕准备移到播放之外）。
            overlay.onlineSubtitleOpen = true
        },
        onProgress = { overlay.moreOpen = false; overlay.calendarOpen = true })
    DetailCalendarSheet(overlay.calendarOpen, d, season, episodes,
        content?.serverName.orEmpty(), content?.loadImage, actions.marks.watchlist,
        onDismiss = { overlay.calendarOpen = false },
        onShare = {
            val text = buildString {
                append(d?.title.orEmpty())
                season?.let { append(" 第${it.index}季") }
                if (episodes.isNotEmpty()) append("·共${episodes.size}集")
                if ((d?.rating ?: 0f) > 0f) append(" 评分${d!!.rating}")
            }
            val send = Intent(Intent.ACTION_SEND).setType("text/plain")
                .putExtra(Intent.EXTRA_TEXT, text)
            openPage(Intent.createChooser(send, "分享"))
        },
        onRemove = { confirmRemove = true })
    if (confirmRemove) {
        DetailConfirmDialog(
            title = "移出待看？",
            message = "这部片会从「待看」列表移除，观看记录不受影响。",
            confirmLabel = "移出",
            onConfirm = { actions.toggle(FilmMark.WATCHLIST) },
            onDismiss = { confirmRemove = false },
        )
    }
    EpisodeGridSheet(overlay.episodeGridOpen,
        season?.name ?: "剧集", episodes,
        onDismiss = { overlay.episodeGridOpen = false },
        // 弹层与集卡同一模型：点集数=选中（主按钮跟随），播放入口统一在主按钮/再点同一集。
        onPlay = onSelectEpisode)
    ResourceVersionSheet(overlay.versionOpen, versions, selectedVersionId,
        loading = versionsLoading, failures = versionFailures,
        onDismiss = { overlay.versionOpen = false }, onSelect = onSelectVersion)
    // 在线字幕：搜索/下载/翻译提前到详情页（翻译分钟级，播放内等待不可行）。
    // videoKey 与播放同口径（NAS 单片 / 本地文件；剧集 path 不一致不预登记，只入库）。
    if (overlay.onlineSubtitleOpen) {
        val activity = context as? android.app.Activity
        val intent = activity?.intent
        val videoKey = if (intent != null) {
            val sourceId = intent.getStringExtra(MovieDetailActivity.EXTRA_SMB_SOURCE)
            val detailPath = intent.getStringExtra(MovieDetailActivity.EXTRA_SMB_PATH)
            val isSeries = intent.getBooleanExtra(MovieDetailActivity.EXTRA_SMB_SERIES, false)
            if (!sourceId.isNullOrBlank() && !detailPath.isNullOrBlank() && !isSeries) {
                val share = intent.getStringExtra(MovieDetailActivity.EXTRA_SMB_SHARE).orEmpty()
                "smb://$sourceId/$share/$detailPath"
            } else {
                intent.getStringExtra(MovieDetailActivity.EXTRA_PATH)
                    ?.takeIf { it.isNotBlank() }?.let { "file:$it" }
            }
        } else null
        OnlineSubtitleSheetFrame(onDismiss = { overlay.onlineSubtitleOpen = false }) {
            OnlineSubtitleSheetContent(
                context = context,
                initialQuery = d?.title.orEmpty(),
                year = d?.date?.take(4)?.toIntOrNull(),
                videoKey = videoKey,
                autoAttach = false,
                onAttach = null,
                onDismiss = { overlay.onlineSubtitleOpen = false },
            )
        }
    }
    // 返回键先关浮层：在线字幕 → 版本 → 集数 → 日历 → 更多 → 选季菜单。
    BackHandler(enabled = overlay.calendarOpen || overlay.moreOpen || menuOpen
        || overlay.episodeGridOpen || overlay.versionOpen || overlay.onlineSubtitleOpen) {
        when {
            overlay.onlineSubtitleOpen -> overlay.onlineSubtitleOpen = false
            overlay.versionOpen -> overlay.versionOpen = false
            overlay.episodeGridOpen -> overlay.episodeGridOpen = false
            overlay.calendarOpen -> overlay.calendarOpen = false
            overlay.moreOpen -> overlay.moreOpen = false
            else -> onMenuClose()
        }
    }
}