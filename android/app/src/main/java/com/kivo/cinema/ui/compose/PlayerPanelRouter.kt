package com.kivo.cinema.ui.compose

import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.player.PlayerPanelData
import com.kivo.cinema.ui.compose.player.PlayerRowControl
import com.kivo.cinema.ui.compose.player.subtitle.handleSubtitleSelect
import com.kivo.cinema.ui.compose.player.subtitle.handleSubtitleStep
import com.kivo.cinema.ui.compose.player.subtitle.resetSubtitlePanel

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
    "audioDelay", "subtitleDisplay", "subtitleTracks", "subtitleExternal", "subtitleSecondary",
    "skipMode", "danmakuSettings",
)

/** stepper 每格步长与小数位（mpv 属性语义，不是 UI 猜测）。字幕步进见 SubtitlePanelActions。 */
private val STEP_UNITS = mapOf(
    "audio-delay" to (0.05 to 2),
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
    // P10：面板内的一次选择也算"用户还在操作" —— 自动隐藏在面板关闭后
    // 从最后一次交互重新计时 3s，而不是从打开面板那一刻计时。
    lastTouchMs = System.currentTimeMillis()
    val panel = activePanel
    // 取证：面板选择的**第一站**。有这条 = 点击到了 Router；没有 = 点击被别处吞掉。
    smbTrace("panelSelect tag=${item.tag} panel=$panel enabled=${item.enabled}")
    if (!item.enabled) {
        showToast(item.subtitle.ifBlank { "该功能暂不可用：${item.title}" })
        return
    }
    if (item.control == PlayerRowControl.CHEVRON && item.tag in DRILL_KINDS) {
        activePanel = item.tag
        smbTrace("panelDrill -> $activePanel")
        return
    }
    // P9：字幕四条面板的写侧整体在 SubtitlePanelActions（Router 只减不增）。
    if (handleSubtitleSelect(panel, item)) return
    // 317 · 选集：季标签只切换面板展示的季；集卡才换集。
    if (item.tag.startsWith("season:")) {
        episodeSeasonPick = item.tag.removePrefix("season:").toIntOrNull() ?: -1
        panelRevision++            // 重新枚举面板，刷新集卡与勾选
        return
    }
    if (item.tag.startsWith("ep:")) {
        // tag 形如 ep:<季下标>:<集下标>
        val parts = item.tag.removePrefix("ep:").split(':')
        val si = parts.getOrNull(0)?.toIntOrNull()
        val ei = parts.getOrNull(1)?.toIntOrNull()
        val blocks = com.kivo.cinema.playback.EpisodeListPayload.decode(
            intent.getStringExtra(com.kivo.cinema.playback.EpisodeListPayload.KEY),
        )
        val entry = si?.let { blocks.getOrNull(it) }?.entries?.getOrNull(ei ?: -1)
        if (entry == null) {
            showToast("该集信息已失效，请返回详情页重进")
            return
        }
        activePanel = ""
        if (item.selected) return          // 点当前集 = 只关面板
        showToast("正在切换到 ${entry.label}")
        episodeSeasonPick = -1
        switchEpisodeImmediately(entry.url, entry.label)
        return
    }
    when {
        item.tag == "reset" -> resetPanel(panel)
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
    lastTouchMs = System.currentTimeMillis()
    val local = LOCAL_STEPS[item.tag]
    if (local != null) {
        stepDanmaku(item.tag, item.step, delta, local)
        return
    }
    if (handleSubtitleStep(item, delta)) return
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
    // P9：字幕重置回到同源出厂值（C++ 初值 / 模型默认 / 面板重置三处一致）。
    if (panel.startsWith("subtitle")) {
        resetSubtitlePanel()
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
        else -> null
    }
}