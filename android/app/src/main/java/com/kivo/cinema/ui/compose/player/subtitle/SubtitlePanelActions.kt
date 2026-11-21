package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.playback.subtitle.SubtitleDelayMemory
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import com.kivo.cinema.playback.subtitle.SubtitleStyle
import com.kivo.cinema.playback.subtitle.SubtitleStyleStore
import com.kivo.cinema.playback.subtitle.subtitlePathKey
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.applySubtitleEncoding
import com.kivo.cinema.ui.compose.flashAction
import com.kivo.cinema.ui.compose.importSubtitleFromClipboard
import com.kivo.cinema.ui.compose.launchSubtitleImport
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.readProperty
import com.kivo.cinema.ui.compose.showToast
import com.kivo.cinema.ui.compose.smbTrace
import com.kivo.cinema.ui.compose.subtitleSourceKey
import java.util.Locale

/**
 * 字幕面板的**事件入口**（P9，从 `PlayerPanelRouter` 整体迁出）。
 *
 * 【为什么不再写在 Router 里】Router 是"选择 → 去向"的路由表；字幕在 P9 长出了
 * 三态 / 调色 / 预设 / 次字幕 / 删除这些**领域规则**，再往路由表里塞，
 * 它就要同时管弹幕、选集、音轨、字幕四套规则（旧版 289 行且还在长）。
 *
 * 本文件只做分派与"删除/重置"两个生命周期动作；样式写路径在
 * [SubtitleStyleActions]，主次轨选择在 [SubtitleTrackActions]，取证钩子在
 * [applySubtitleProbeWrites]。返回 true = 已处理，Router 不再看这个事件。
 *
 * 【线程纪律】内核写一律在后台线程；结果回主线程 toast + `panelRevision++`
 * （读侧只认 revision，否则步进后数字与勾选不动）。
 */
internal val SUBTITLE_PANELS = setOf(
    // 入口面板也必须在内：`导入字幕`(sub-import)/`关闭字幕`(sub-off) 只出现在入口面板，
    // 漏掉 kind="subtitle" 会让两行在 Router 里被静默吞掉（P9 返工 FAIL-3 根因）。
    "subtitle", "subtitleDisplay", "subtitleTracks", "subtitleExternal", "subtitleSecondary",
    "subtitleEncoding",
)
private val TRACK_PANELS = setOf("subtitleTracks", "subtitleExternal")

/**
 * 编码面板的 7 个 tag（`sub-enc-auto` / `sub-enc-UTF-8` / …）；后缀就是
 * 存储层认的编排名，`auto` 表示交回自动探测。
 */
private val SUBTITLE_ENCODING_TAGS = setOf(
    "sub-enc-auto",
    "sub-enc-UTF-8",
    "sub-enc-GB18030",
    "sub-enc-BIG5",
    "sub-enc-Shift-JIS",
    "sub-enc-UTF-16LE",
    "sub-enc-UTF-16BE",
)

/** 调色板循环行（写样式 + 落盘）；与 stepper 分开，值不是数字。 */
internal val SUBTITLE_COLOR_TAGS = setOf("sub-color", "sub-outline-color", "sub-back-color")

private data class Step(val step: Double, val digits: Int, val min: Double, val max: Double)

/** stepper 步长与区间；`sub-pos` 上限 100（>100 会被 libass 裁掉，不是"更高"）。 */
private val SUBTITLE_STEPS: Map<String, Step> = mapOf(
    "sub-font-size" to Step(1.0, 0, SubtitleStyle.MIN_FONT.toDouble(), SubtitleStyle.MAX_FONT.toDouble()),
    "sub-pos" to Step(1.0, 0, 0.0, SubtitleStyle.MAX_POS.toDouble()),
    "sub-margin-y" to Step(1.0, 0, 0.0, SubtitleStyle.MAX_MARGIN_Y.toDouble()),
    "sub-border-size" to Step(0.5, 1, 0.0, SubtitleStyle.MAX_BORDER),
    "sub-shadow-offset" to Step(0.5, 1, 0.0, SubtitleStyle.MAX_SHADOW),
    "sub-spacing" to Step(0.5, 1, SubtitleStyle.MIN_SPACING, SubtitleStyle.MAX_SPACING),
    "sub-delay" to Step(0.1, 1, -SubtitleStyle.MAX_DELAY_SECONDS, SubtitleStyle.MAX_DELAY_SECONDS),
    "secondary-sub-pos" to Step(1.0, 0, 0.0, SubtitleStyle.MAX_POS.toDouble()),
    "secondary-sub-delay" to Step(0.1, 1, -SubtitleStyle.MAX_DELAY_SECONDS, SubtitleStyle.MAX_DELAY_SECONDS),
)

/** @return true = 本事件属于字幕面板，已处理（Router 直接返回）。 */
internal fun PlayerComposeActivity.handleSubtitleSelect(
    panel: String,
    item: PlayerOptionItem,
): Boolean {
    if (panel !in SUBTITLE_PANELS) return false
    when {
        item.tag == "sub-import" -> launchSubtitleImport()
        item.tag == "sub-import-url" -> importSubtitleFromClipboard()
        // 「字幕编码」是钻入行：只换面板，不碰内核（与 Router 的 CHEVRON 同口径）。
        item.tag == "subtitleEncoding" -> {
            activePanel = item.tag
            smbTrace("panelDrill -> $activePanel")
        }
        item.tag in SUBTITLE_ENCODING_TAGS -> applySubtitleEncoding(item.tag.removePrefix("sub-enc-"))
        item.tag == "sub-off" -> writeSubtitleOff()
        item.tag == "sub-ass-mode" || item.tag == "sub-ass-override" -> cycleAssMode()
        item.tag.startsWith("sub-preset-") -> {
            val size = item.tag.removePrefix("sub-preset-").toIntOrNull()
            if (size == null) showToast("字号档位无效") else applyFontPreset(size)
        }
        item.tag == "sub-bold" -> toggleBold()
        item.tag in SUBTITLE_COLOR_TAGS -> cycleColor(item.tag)
        item.tag == "sub-border-style" -> cycleBorderStyle()
        item.tag == "secondary-sub-visibility" -> toggleSecondaryVisible()
        item.tag == "secondary:no" -> closeSecondary()
        item.tag.startsWith("secondary:") -> pickSecondary(item.tag.removePrefix("secondary:"))
        panel in TRACK_PANELS -> pickMainTrack(panel, item)
        else -> return false
    }
    return true
}

/** 同一步进器连续点击的串行闸：读-算-写必须原子，否则并发线程各自读到旧值会丢步。 */
private val subtitleStepLock = Any()

/**
 * @return true = 字幕 stepper，已处理。
 *
 * 【基准值】步进在**写入线程里读内核当前值**再算，不用行快照（`item.step`）：
 * 连点快于重组 + IO 重算时，快照会停在同一帧 —— 表现为"只生效一次"或
 * 停在中途（P9 返工 FAIL-1 写侧根因）。读不到才退回快照。
 */
internal fun PlayerComposeActivity.handleSubtitleStep(item: PlayerOptionItem, delta: Int): Boolean {
    val spec = SUBTITLE_STEPS[item.tag] ?: return false
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull()
    if (mpv == null || native == null) {
        showToast("内核尚未就绪，暂时无法调整")
        return true
    }
    Thread {
        // 内核拒绝时要能指出**哪一条**没写进去；lastError 常常是空串（旧版提示因此只剩"调整失败："）。
        var rejected = "${item.tag}=${item.value}"
        val ok = synchronized(subtitleStepLock) {
            val current = mpv.readProperty(item.tag)?.trim()?.toDoubleOrNull() ?: item.step
            val next = (current + delta * spec.step).coerceIn(spec.min, spec.max)
            val text = String.format(Locale.US, "%.${spec.digits}f", next)
            val written = native.setProperty(item.tag, text)
            if (written) persistNumeric(item.tag, next, native) else rejected = "${item.tag}=$text"
            written
        }
        runOnUiThread {
            if (ok) panelRevision++ else showToast(rejectText(rejected, mpv.lastError))
        }
    }.start()
    return true
}

/** 写失败提示（与 `SubtitleStyleActions.writeSubtitleStyle` 同口径）：先点名 tag=value，lastError 非空才附带。 */
private fun rejectText(tagValue: String, lastError: String?): String {
    val detail = lastError?.trim().orEmpty()
    return if (detail.isBlank()) "调整失败：内核拒绝 $tagValue" else "调整失败：内核拒绝 $tagValue（$detail）"
}

/** 重置：内核 + 落盘同时回到**同源**出厂值（P9 · A：不再写 55）。 */
internal fun PlayerComposeActivity.resetSubtitlePanel() {
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull()
    if (mpv == null || native == null) {
        showToast("内核尚未就绪，暂时无法重置")
        return
    }
    Thread {
        // 顺序：先写内核，**全部成功才落盘** —— 旧版先落盘再写内核，
        // 内核拒绝时 prefs 已是出厂值、画面却是旧的，重启后两边对不上。
        // 整段与步进共用 [subtitleStepLock]：重置写内核期间若有步进插进来，
        // 它会读到「一半出厂值一半旧值」再叠加 delta 写回，盘上留下混血状态。
        // 锁序与步进一致（stepLock → SubtitleStyleStore.ioLock，reset 内部只取 ioLock），无反向。
        val factory = SubtitleStyle()
        val (failed, delayOk) = synchronized(subtitleStepLock) {
            val failedTags = factory.properties()
                .filterNot { native.setProperty(it.first, it.second) }
                .map { it.first }
            val delayWritten = native.setProperty("sub-delay", "0")
            // SUB-M5：次字幕延迟与主延迟一起回 0，否则"恢复默认"只清了一半。
            val secondaryDelayWritten = native.setProperty("secondary-sub-delay", "0")
            if (failedTags.isEmpty() && delayWritten && secondaryDelayWritten) {
                SubtitleStyleStore(this).reset()
                val memory = SubtitleDelayMemory(this)
                memory.save(subtitleSourceKey(), 0.0)
                memory.saveSecondary(subtitleSourceKey(), 0.0)
            }
            failedTags to (delayWritten && secondaryDelayWritten)
        }
        val allOk = failed.isEmpty() && delayOk
        runOnUiThread {
            // 无论成败都刷新：失败项的内核值可能已部分变化，行快照必须重读。
            panelRevision++
            if (allOk) {
                showToast("字幕样式已恢复默认")
            } else {
                val names = failed + if (delayOk) emptyList()
                else listOf("sub-delay", "secondary-sub-delay")
                showToast("重置部分失败：${names.joinToString(",")}（已成功的项未落盘）")
            }
        }
    }.start()
}

/** 长按外挂字幕行（H）：确认后 sub-remove + 只清理**本片源**的导入文件。 */
internal fun PlayerComposeActivity.onSubtitleLongPress(item: PlayerOptionItem) {
    lastTouchMs = System.currentTimeMillis()
    if (activePanel != "subtitleExternal") return
    val id = item.tag.toIntOrNull() ?: return
    val track = mpvNetSession?.tracksOrNull()?.subtitleTracks()
        ?.firstOrNull { it.id == id } ?: return
    if (!track.external) return
    val title = SubtitleTrackLabels.title(track)
    // 文件归属只认内核给的**精确路径**：空 = 无法确认，删除时只移除轨道、保留文件。
    val filePath = track.externalFilename
    val cleanupHint = if (filePath.isBlank()) {
        "无法确认文件归属，将只移除轨道并保留文件。"
    } else {
        "并清理本片源导入的字幕文件。"
    }
    android.app.AlertDialog.Builder(this)
        .setTitle("删除外挂字幕")
        .setMessage("将从当前播放移除「$title」，$cleanupHint")
        .setPositiveButton("删除") { _, _ -> deleteExternalSubtitle(id, filePath) }
        .setNegativeButton("取消", null)
        .show()
}

private fun PlayerComposeActivity.deleteExternalSubtitle(id: Int, filePath: String) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法删除")
        return
    }
    Thread {
        // M2：先数**删除前**还有几条外挂轨指向同一文件。>1 ⇒ 删掉这条后文件仍被使用，
        // 此时删盘会让剩下的轨变孤儿；登记也保留，等最后一条轨删除时再清理。
        // R4：路径两侧统一走 subtitlePathKey —— mpv 的 /data/data/… 与登记时的
        // /data/user/0/… 是同一文件，逐字符比会把「共用文件」漏成 0/1（误删或误留）。
        val fileKey = subtitlePathKey(filePath)
        val sharedBefore = if (filePath.isBlank()) 0 else {
            mpv.tracksOrNull()?.subtitleTracks()
                ?.count { it.external && subtitlePathKey(it.externalFilename) == fileKey } ?: 0
        }
        val result = mpv.removeExternalSubtitle(id)
        if (!result.removed) {
            smbTrace("sub-remove failed id=$id err=${mpv.lastError}")
            runOnUiThread {
                panelRevision++
                showToast("删除失败：${mpv.lastError}")
            }
            return@Thread
        }
        val store = SubtitleImportStore(this) { smbTrace(it) }
        val key = subtitleSourceKey()
        val file = if (filePath.isBlank() || sharedBefore != 1) null
        else store.findRegisteredByPath(key, filePath)
        val deleted = if (file != null) store.forget(key, file) else false
        val message = when {
            filePath.isBlank() -> "已移除字幕轨；无法确认文件归属，仅移除轨道"
            sharedBefore > 1 -> "已移除外挂字幕（文件仍被其他轨道使用）"
            sharedBefore == 0 -> "已移除外挂字幕（文件归属已变化，文件已保留）"
            deleted -> "已删除外挂字幕与导入文件"
            else -> "已移除外挂字幕"
        } + (result.warning?.let { "；$it" } ?: "")
        // M4 / O2 取证：删完之后**内核自选了谁**由 sid / sec 两项回答（兜底可能已改写 sid）。
        smbTrace(
            "sub-remove ok id=$id shared=$sharedBefore file=${file?.name} disk=$deleted " +
                "sid=${mpv.readProperty("sid")} sec=${mpv.readProperty("secondary-sid")} " +
                "warn=${result.warning}",
        )
        runOnUiThread {
            panelRevision++
            showToast(message)
        }
    }.start()
}

/** 关闭字幕＝sid=no（不是删轨）。面板不关：副题会重读成"关闭"。 */
private fun PlayerComposeActivity.writeSubtitleOff() {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法关闭字幕")
        return
    }
    Thread {
        val ok = mpv.disableSubtitles()
        runOnUiThread {
            if (ok) {
                panelRevision++
                showToast("字幕已关闭")
            } else {
                showToast("关闭字幕失败：${mpv.lastError}")
            }
        }
    }.start()
}
