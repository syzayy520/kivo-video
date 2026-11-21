package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.playback.subtitle.SubtitleBorderStyle
import com.kivo.cinema.playback.subtitle.SubtitleDelayMemory
import com.kivo.cinema.playback.subtitle.SubtitleStyle
import com.kivo.cinema.playback.subtitle.SubtitleStyleStore
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.readProperty
import com.kivo.cinema.ui.compose.showToast
import com.kivo.cinema.ui.compose.subtitleSourceKey

/**
 * **样式写路径**（P9 · A/B/C/E/F）：ASS 三态、字号预设、调色、边框、加粗、
 * 次字幕显隐，以及所有"内核 → 落盘 → 回读"的统一写法。
 *
 * 【一条铁律】面板上的值来自内核（读侧真值），写只走这里 ⇒ 内核与
 * `SubtitleStyleStore` 永远同步；重启后起播又用同一份样式（[SubtitleStyle.properties]
 * 是唯一属性表）。失败一律 toast，不许"点了没反应"。
 */
internal fun PlayerComposeActivity.cycleAssMode() {
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪")
    if (mpv.nativeOrNull() == null) return showToast("内核尚未就绪")
    val next = SubtitleStyle.assModeNext(SubtitleStyle.assModeOf(mpv.readProperty("sub-ass-override")))
    writeSubtitleStyle("sub-ass-override", next.mpvValue, { it.copy(assMode = next) }, "ASS 字幕样式：${next.label}")
}

internal fun PlayerComposeActivity.applyFontPreset(size: Int) {
    val preset = SubtitleStyle.FONT_PRESETS.firstOrNull { it.size == size } ?: return
    writeSubtitleStyle(
        "sub-font-size",
        preset.size.toString(),
        { it.copy(fontSize = preset.size) },
        "字号：${preset.label}",
    )
}

internal fun PlayerComposeActivity.toggleBold() {
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪")
    if (mpv.nativeOrNull() == null) return showToast("内核尚未就绪")
    val next = mpv.readProperty("sub-bold") != "yes"
    writeSubtitleStyle(
        "sub-bold",
        if (next) "yes" else "no",
        { it.copy(bold = next) },
        if (next) "字幕已加粗" else "字幕已取消加粗",
    )
}

internal fun PlayerComposeActivity.cycleColor(tag: String) {
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪")
    if (mpv.nativeOrNull() == null) return showToast("内核尚未就绪")
    val palette = when (tag) {
        "sub-color" -> SubtitleStyle.COLORS
        "sub-outline-color" -> SubtitleStyle.OUTLINE_COLORS
        else -> SubtitleStyle.BACK_COLORS
    }
    val next = SubtitleStyle.cycleColor(palette, mpv.readProperty(tag).orEmpty())
    val persist: (SubtitleStyle) -> SubtitleStyle = {
        when (tag) {
            "sub-color" -> it.copy(color = next.value)
            "sub-outline-color" -> it.copy(outlineColor = next.value)
            else -> it.copy(backColor = next.value)
        }
    }
    writeSubtitleStyle(tag, next.value, persist, "${colorTitle(tag)}：${next.label}")
}

private fun colorTitle(tag: String): String = when (tag) {
    "sub-color" -> "文字颜色"
    "sub-outline-color" -> "描边颜色"
    else -> "背景框颜色"
}

internal fun PlayerComposeActivity.cycleBorderStyle() {
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪")
    if (mpv.nativeOrNull() == null) return showToast("内核尚未就绪")
    val raw = mpv.readProperty("sub-border-style").orEmpty().trim().lowercase()
    val current = SubtitleBorderStyle.entries.firstOrNull { it.mpvValue == raw } ?: SubtitleBorderStyle.OUTLINE
    val next = SubtitleBorderStyle.entries[(current.ordinal + 1).mod(SubtitleBorderStyle.entries.size)]
    writeSubtitleStyle("sub-border-style", next.mpvValue, { it.copy(borderStyle = next) }, "边框样式：${next.label}")
}

internal fun PlayerComposeActivity.toggleSecondaryVisible() {
    val mpv = mpvNetSession ?: return showToast("内核尚未就绪")
    if (mpv.nativeOrNull() == null) return showToast("内核尚未就绪")
    val next = mpv.readProperty("secondary-sub-visibility") == "no"
    writeSubtitleStyle(
        "secondary-sub-visibility",
        if (next) "yes" else "no",
        { it.copy(secondaryVisible = next) },
        if (next) "次字幕已显示" else "次字幕已隐藏",
    )
}

/** 统一样式写：内核 → 落盘 → 回读。失败必须让用户看见。 */
internal fun PlayerComposeActivity.writeSubtitleStyle(
    tag: String,
    value: String,
    persist: ((SubtitleStyle) -> SubtitleStyle)?,
    success: String?,
) {
    val mpv = mpvNetSession
    val native = mpv?.nativeOrNull()
    if (mpv == null || native == null) {
        showToast("内核尚未就绪，暂时无法调整")
        return
    }
    Thread {
        val ok = native.setProperty(tag, value)
        if (ok && persist != null) SubtitleStyleStore(this).update(persist)
        runOnUiThread {
            if (!ok) {
                showToast("调整失败：内核拒绝 $tag=$value")
            } else {
                panelRevision++
                if (success != null) showToast(success)
            }
        }
    }.start()
}

/** 数值步进的分流落盘：延迟按片源记住，其余进全局样式。 */
internal fun PlayerComposeActivity.persistNumeric(tag: String, value: Double, native: MpvSession) {
    when (tag) {
        "sub-delay" -> SubtitleDelayMemory(this).save(subtitleSourceKey(), value)
        // SUB-M5：内核值已由步进/探针写入（[handleSubtitleStep] / [applySubtitleProbeWrites]），
        // 这里只补"按片源记忆"—— 不再重复 setProperty，否则同值会被写两次（实测 0.1 / 0.10）。
        "secondary-sub-delay" -> SubtitleDelayMemory(this).saveSecondary(subtitleSourceKey(), value)
        else -> SubtitleStyleStore(this).update { it.withNumeric(tag, value) }
    }
}

/** 样式数值字段表（不含两个延迟）——步进与取证钩子共用同一集合。 */
internal val STYLE_NUMERIC: Set<String> = setOf(
    "sub-font-size", "sub-pos", "sub-margin-y", "sub-border-size",
    "sub-shadow-offset", "sub-spacing", "secondary-sub-pos",
)

private fun SubtitleStyle.withNumeric(tag: String, value: Double): SubtitleStyle = when (tag) {
    "sub-font-size" -> copy(fontSize = SubtitleStyle.clampFontSize(value))
    "sub-pos" -> copy(pos = SubtitleStyle.clampPos(value))
    "sub-margin-y" -> copy(marginY = SubtitleStyle.clampMarginY(value))
    "sub-border-size" -> copy(borderSize = SubtitleStyle.clampBorderSize(value))
    "sub-shadow-offset" -> copy(shadowOffset = SubtitleStyle.clampShadowOffset(value))
    "sub-spacing" -> copy(spacing = SubtitleStyle.clampSpacing(value))
    "secondary-sub-pos" -> copy(secondaryPos = SubtitleStyle.clampPos(value))
    else -> this
}
