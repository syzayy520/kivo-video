package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.playback.subtitle.SubtitleSecondaryControl
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.flashAction
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.readProperty
import com.kivo.cinema.ui.compose.showToast

/**
 * **主 / 次字幕选轨**（P9 · F/G）。
 *
 * mpv 的两个字幕位（`sid` / `secondary-sid`）互相独立但**不许同轨** ——
 * 同一条轨同时占据两个位会在两个位置叠印同一句（重影）。
 * 规则集中在这里：选主轨时若撞上次字幕，先关次字幕再切，并明确告知用户。
 */
internal fun PlayerComposeActivity.pickMainTrack(panel: String, item: PlayerOptionItem) {
    val id = item.tag.toIntOrNull() ?: return showToast("字幕参数无效")
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪，暂时无法切换")
    Thread {
        // 主/次不能同轨：主轨点到次字幕那条时先关次字幕（并告知），否则会重影。
        // 关次字幕失败必须**中止**：硬着头皮切主轨会留下两轨叠印，比"没切成"更糟。
        val sameAsSecondary = mpv.readProperty("secondary-sid")?.trim()?.toIntOrNull() == id
        if (sameAsSecondary) {
            val native = mpv.nativeOrNull()
            val closed = native != null && SubtitleSecondaryControl.disable(native)
            if (!closed) {
                runOnUiThread { showToast("未能关闭次字幕，切换已取消") }
                return@Thread
            }
        }
        val ok = mpv.selectSubtitle(id)
        runOnUiThread {
            if (ok) {
                activePanel = ""
                panelRevision++
                flashAction(panel)
                if (sameAsSecondary) showToast("已关闭次字幕（与主字幕相同）")
            } else {
                showToast("字幕切换失败：${mpv.lastError}")
            }
        }
    }.start()
}

internal fun PlayerComposeActivity.pickSecondary(raw: String) {
    val id = raw.toIntOrNull() ?: return showToast("次字幕参数无效")
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull()
    if (mpv == null || native == null) {
        showToast("内核尚未就绪，暂时无法切换")
        return
    }
    Thread {
        val catalog = mpv.tracksOrNull()
        val mainId = catalog?.currentTrackId("sub") ?: -1
        val error = SubtitleSecondaryControl.select(native, id, mainId)
        val label = catalog?.subtitleTracks()?.firstOrNull { it.id == id }
            ?.let { SubtitleTrackLabels.title(it) } ?: "id=$id"
        runOnUiThread {
            if (error == null) {
                activePanel = ""
                panelRevision++
                showToast("次字幕：$label")
            } else {
                showToast(error)
            }
        }
    }.start()
}

internal fun PlayerComposeActivity.closeSecondary() {
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull() ?: return showToast("内核尚未就绪")
    Thread {
        val ok = SubtitleSecondaryControl.disable(native)
        // 内核拒绝时 lastError 常为空：至少点名是哪条属性没写进去（同 L2 口径）。
        val detail = mpv?.lastError?.trim().orEmpty()
        runOnUiThread {
            if (ok) {
                panelRevision++
                showToast("次字幕已关闭")
            } else {
                showToast(
                    if (detail.isBlank()) "关闭次字幕失败：内核拒绝 secondary-sid=no"
                    else "关闭次字幕失败：内核拒绝 secondary-sid=no（$detail）",
                )
            }
        }
    }.start()
}
