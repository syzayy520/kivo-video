package com.kivo.cinema.ui.compose

import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.player.PlayerPanelData
import com.kivo.cinema.ui.compose.player.PlayerRowControl

/**
 * 面板选择 → 内核写入（v3 §1.4 十三形态的写侧）。
 *
 * 【为什么与取数分开】取数是"读"（可并发、失败只影响显示），
 * 写入是"改"（同步拿核心锁、失败必须让用户看见）。线程策略与失败语义都不同。
 *
 * 【三类行三种去向】
 * · CHEVRON 钻入行 → 只换 activePanel，不碰内核；
 * · STEPPER/TOGGLE → 写一个 mpv 属性 / 一个本地态；
 * · 普通行 → 切轨/切倍速/切比例。
 * 写完一律 `panelRevision++`，让 [rememberPanelItems] 重算 ——
 * 否则 stepper 按了 + 面板上的数字不动（读侧没有别的刷新信号）。
 */

/** 钻入关系：父面板里的行 tag = 子面板 kind（§1.4 的 1→内嵌/外挂/显示、4→5、6→7、8→9）。 */
private val DRILL_KINDS = setOf(
    "audioDelay", "subtitleDisplay", "subtitleTracks", "subtitleExternal",
    "skipMode", "danmakuSettings",
)

/** stepper 每格步长与小数位（mpv 属性语义，不是 UI 猜测）。 */
private val STEP_UNITS = mapOf(
    "audio-delay" to (0.05 to 2),
    "sub-delay" to (0.1 to 2),
    "sub-font-size" to (1.0 to 0),
    "sub-pos" to (1.0 to 0),
    "sub-border-size" to (0.5 to 1),
)

/**
 * 本地 stepper（v3.1）：弹幕是 Compose 自绘层，mpv 没有 danmaku-* 属性 ——
 * 旧版把 danmaku-speed 混进 STEP_UNITS 写 mpv，每次必失败。
 * 三元组 = (步长, 下限, 上限)；不透明度以百分数步进、写回时除 100。
 */
private val LOCAL_STEPS = mapOf(
    "danmaku-speed" to Triple(0.5, 4.0, 16.0),
    "danmaku-font" to Triple(0.1, 0.6, 1.6),
    "danmaku-opacity" to Triple(10.0, 30.0, 100.0),
)

internal fun PlayerComposeActivity.onPanelSelect(item: PlayerOptionItem) {
    if (!item.enabled) {
        showToast(item.subtitle.ifBlank { "该功能暂不可用：${item.title}" })
        return
    }
    val panel = activePanel
    if (item.control == PlayerRowControl.CHEVRON && item.tag in DRILL_KINDS) {
        activePanel = item.tag
        return
    }
    when {
        item.tag == "reset" -> resetPanel(panel)
        item.tag == "sub-ass-override" -> writeSubtitleOverride(item.on)
        item.tag == "sub-off" -> writeSubtitleOff()
        item.tag == "sub-import" -> launchSubtitleImport()
        item.tag == "danmaku" -> {
            // 铁律：没有弹幕文件时不许"开了但什么都不显示"。
            if (!danmakuOn && !danmakuSettings.hasItems) {
                showToast("未找到弹幕文件（danmaku/<片名>.xml）")
                return
            }
            danmakuOn = !danmakuOn
            panelRevision++
            showToast(if (danmakuOn) "弹幕已开启" else "弹幕已关闭")
        }
        item.tag.startsWith("danmaku-area-") -> {
            val area = when (item.tag) {
                "danmaku-area-top" -> com.kivo.cinema.danmaku.DanmakuArea.TOP_HALF
                "danmaku-area-bottom" -> com.kivo.cinema.danmaku.DanmakuArea.BOTTOM_HALF
                else -> com.kivo.cinema.danmaku.DanmakuArea.FULL
            }
            danmakuSettings = danmakuSettings.copy(area = area)
            panelRevision++
        }
        item.section == "chips" -> {
            //  chips 只列内核真报的那一路渲染；点它 = 确认当前资源。
            activePanel = ""
            flashAction("resource")
        }
        else -> writeSelection(panel, item)
    }
}

/** stepper 的 −/+：当前值取自内核真值（item.step），不本地累加猜。 */
internal fun PlayerComposeActivity.onPanelStep(item: PlayerOptionItem, delta: Int) {
    val local = LOCAL_STEPS[item.tag]
    if (local != null) {
        stepDanmaku(item.tag, item.step, delta, local)
        return
    }
    val mpv = mpvNetSession
    val unit = STEP_UNITS[item.tag]
    if (mpv == null || unit == null) {
        showToast("内核尚未就绪，暂时无法调整")
        return
    }
    val (step, digits) = unit
    val next = item.step + delta * step
    Thread {
        val ok = mpv.writeProperty(item.tag, String.format("%.${digits}f", next))
        runOnUiThread {
            if (ok) panelRevision++ else showToast("调整失败：${mpv.lastError}")
        }
    }.start()
}

/** 弹幕本地 stepper：夹在合法区间内直写本地态，无需内核、无失败路径。 */
private fun PlayerComposeActivity.stepDanmaku(
    tag: String,
    current: Double,
    delta: Int,
    spec: Triple<Double, Double, Double>,
) {
    val (step, min, max) = spec
    val next = (current + delta * step).coerceIn(min, max)
    danmakuSettings = when (tag) {
        "danmaku-speed" -> danmakuSettings.copy(speedSec = next.toFloat())
        "danmaku-font" -> danmakuSettings.copy(fontScale = next.toFloat())
        else -> danmakuSettings.copy(opacity = (next / 100).toFloat())
    }
    panelRevision++
}

/** 重置 = 回到默认值（不是回到"打开面板时的值"）。弹幕是本地态，不依赖内核。 */
private fun PlayerComposeActivity.resetPanel(panel: String) {
    if (panel == "danmakuSettings") {
        danmakuSettings = com.kivo.cinema.danmaku.DanmakuSettings(hasItems = danmakuSettings.hasItems)
        panelRevision++
        return
    }
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法重置")
        return
    }
    Thread {
        val ok = when (panel) {
            "audioDelay" -> mpv.writeProperty("audio-delay", "0")
            "subtitleDisplay" -> mpv.writeProperty("sub-font-size", "55") &&
                mpv.writeProperty("sub-pos", "100") && mpv.writeProperty("sub-delay", "0") &&
                mpv.writeProperty("sub-border-size", "3") &&
                mpv.writeProperty("sub-ass-override", "no")
            else -> false
        }
        runOnUiThread {
            if (ok) panelRevision++ else showToast("重置失败：${mpv.lastError}")
        }
    }.start()
}

/** 听视频态：vid=no 关视频轨、音频继续；恢复 = vid=auto。 */
internal fun PlayerComposeActivity.setAudioOnly(on: Boolean) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法切换听视频")
        return
    }
    Thread {
        val ok = mpv.writeProperty("vid", if (on) "no" else "auto")
        runOnUiThread {
            if (ok) {
                audioOnly = on
                activePanel = ""
                panelRevision++
            } else {
                showToast("听视频切换失败：${mpv.lastError}")
            }
        }
    }.start()
}

/**
 * 标准统一样式开关（§1.6）：yes＝sub-* 统一样式压过 ASS 源样式，
 * no＝保留 ASS 源特效（默认）。面板留在原处，只重读。
 */
private fun PlayerComposeActivity.writeSubtitleOverride(currentlyOn: Boolean) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法切换字幕样式")
        return
    }
    Thread {
        val ok = mpv.writeProperty("sub-ass-override", if (currentlyOn) "no" else "yes")
        runOnUiThread {
            if (ok) panelRevision++ else showToast("字幕样式切换失败：${mpv.lastError}")
        }
    }.start()
}

/** 关闭字幕＝sid=no。面板不关：内嵌字幕行的副题会重读成"关闭"。 */
private fun PlayerComposeActivity.writeSubtitleOff() {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法关闭字幕")
        return
    }
    Thread {
        val ok = mpv.writeProperty("sid", "no")
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

private fun PlayerComposeActivity.writeSelection(panel: String, item: PlayerOptionItem) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法切换")
        return
    }
    Thread {
        val error = applyPanelSelection(panel, item, mpv)
        runOnUiThread {
            if (error == null) {
                activePanel = ""
                panelRevision++
                flashAction(panel)
            } else {
                showToast(error)
            }
        }
    }.start()
}

/** @return null = 成功；否则是给用户看的失败原因（非空、可见）。 */
private fun applyPanelSelection(
    panel: String,
    item: PlayerOptionItem,
    mpv: MpvPlaybackSession,
): String? {
    return when (panel) {
        "speed" -> {
            val value = PlayerPanelData.speedValue(item.title)
            if (value == null) "倍速档位不合法：${item.title}"
            else if (mpv.setSpeed(value)) null else "倍速设置失败：${mpv.lastError}"
        }
        // video-aspect-override 直接吃 "16:9"/"4:3"/"2.35:1"/"Original"。
        "quality" -> if (mpv.writeProperty("video-aspect-override", item.tag)) null
        else "画质比例设置失败：${mpv.lastError}"
        "audio" -> {
            val id = item.tag.toIntOrNull() ?: return "音轨参数无效"
            val catalog = mpv.tracksOrNull() ?: return "内核尚未就绪"
            if (catalog.selectTrack("audio", id)) null else "音轨切换失败：${mpv.lastError}"
        }
        // 字幕轨道行住在两个钻入页里（内嵌/外挂），写法相同：sid=<id>。
        "subtitleTracks", "subtitleExternal" -> {
            val id = item.tag.toIntOrNull() ?: return "字幕参数无效"
            if (mpv.selectSubtitle(id)) null else "字幕切换失败：${mpv.lastError}"
        }
        else -> null
    }
}