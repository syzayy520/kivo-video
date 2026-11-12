package com.kivo.cinema.ui.detail

import android.content.Intent
import android.widget.Toast
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.media.marks.FilmMark
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import com.kivo.cinema.ui.detail.actions.DetailActions
import com.kivo.cinema.ui.detail.actions.DetailMorePopup

/** 255 #31 浮层状态：更多菜单锚点/开合 + 日历开合。DetailPage 只持有一个控制器。 */
internal class DetailOverlayState {
    var moreOpen by mutableStateOf(false)
    var moreAnchor by mutableStateOf(Rect.Zero)
    var calendarOpen by mutableStateOf(false)
    fun openMore(anchor: Rect) { moreAnchor = anchor; moreOpen = true }
}

/**
 * 「更多」毛玻璃菜单 + 追剧日历浮层的编排宿主（从 DetailPage 抽出，控行数）。
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
    backdrop: NavigationBackdropState,
    openPage: (Intent) -> Unit,
) {
    val context = LocalContext.current
    val d = content?.details
    DetailMorePopup(overlay.moreOpen, overlay.moreAnchor, backdrop,
        onDismiss = { overlay.moreOpen = false },
        onAddCalendar = {
            overlay.moreOpen = false
            if (!actions.marks.watchlist) actions.toggle(FilmMark.WATCHLIST)
            overlay.calendarOpen = true
        },
        onDownload = {
            overlay.moreOpen = false
            Toast.makeText(context, "下载功能尚未开放，敬请期待", Toast.LENGTH_LONG).show()
        },
        onProgress = { overlay.moreOpen = false; overlay.calendarOpen = true })
    DetailCalendarSheet(overlay.calendarOpen, d, season, episodes,
        content?.serverName.orEmpty(), backdrop, content?.loadImage, actions.marks.watchlist,
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
        onRemove = { actions.toggle(FilmMark.WATCHLIST) })
}