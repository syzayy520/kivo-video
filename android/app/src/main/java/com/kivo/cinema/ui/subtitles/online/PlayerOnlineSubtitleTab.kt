package com.kivo.cinema.ui.subtitles.online

import androidx.compose.runtime.Composable
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import com.kivo.cinema.playback.subtitle.SubtitleSourceKey
import com.kivo.cinema.subtitle.online.OnlineSubtitle
import com.kivo.cinema.subtitle.online.OnlineSubtitlePrefs
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.attachImported
import com.kivo.cinema.ui.compose.currentTitle
import com.kivo.cinema.ui.compose.showToast
import com.kivo.cinema.ui.compose.subtitleSourceKey
import java.io.File

/**
 * 播放侧薄壳（PlayerComposeActivity 扩展）：通用面板 + 播放挂载回调。
 * 详情页共用同一面板内容（见 DetailOnlineSubtitle.kt）。
 */
@Composable
internal fun PlayerComposeActivity.OnlineSubtitleTab(onClose: () -> Unit) {
    OnlineSubtitleSheetContent(
        context = this,
        initialQuery = currentTitle(),
        year = null,
        videoKey = subtitleSourceKey(),
        autoAttach = true,
        onAttach = { imported: SubtitleImportStore.Imported ->
            val failure = attachImported(imported, requireNotNull(mpvNetSession))
            if (failure != null) {
                showToast(failure)
                return@OnlineSubtitleSheetContent
            }
            panelRevision++
        },
        onDismiss = onClose,
    )
}