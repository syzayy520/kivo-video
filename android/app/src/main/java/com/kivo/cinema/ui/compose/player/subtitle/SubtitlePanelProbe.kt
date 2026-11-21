package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.playback.subtitle.SubtitleAssMode
import com.kivo.cinema.playback.subtitle.SubtitleBorderStyle
import com.kivo.cinema.playback.subtitle.SubtitleStyle
import com.kivo.cinema.playback.subtitle.SubtitleStyleStore
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.smbTrace

/**
 * 取证钩子：`--es kivo.subWrite "sub-font-size=48,sub-color=#FFFFD400"`
 *
 * 走与面板**同一条**落盘 + 下发路径（不含面板动画），供真机 A/B 取证；
 * 未知键跳过并留痕 —— 不许假装成功。每组只改一处 → 截图对比 →
 * 重启进程复播可验证 C 组（持久化是否生效）。
 */
internal fun PlayerComposeActivity.applySubtitleProbeWrites(spec: String, native: MpvSession) {
    spec.split(',').map { it.trim() }.filter { it.isNotEmpty() }.forEach { token ->
        val name = token.substringBefore('=').trim()
        val value = token.substringAfter('=', "").trim()
        val ok = when {
            name in STYLE_NUMERIC || name == "sub-delay" || name == "secondary-sub-delay" ->
                native.setProperty(name, value).also { written ->
                    if (written) persistNumeric(name, value.toDoubleOrNull() ?: 0.0, native)
                }
            name in SUBTITLE_COLOR_TAGS -> native.setProperty(name, value).also { written ->
                if (written) SubtitleStyleStore(this).update {
                    when (name) {
                        "sub-color" -> it.copy(color = SubtitleStyle.normalizeColor(value, it.color))
                        "sub-outline-color" ->
                            it.copy(outlineColor = SubtitleStyle.normalizeColor(value, it.outlineColor))
                        else -> it.copy(backColor = SubtitleStyle.normalizeColor(value, it.backColor))
                    }
                }
            }
            name == "sub-ass-override" || name == "sub-ass-mode" -> {
                val mode = if (name == "sub-ass-mode") {
                    runCatching { SubtitleAssMode.valueOf(value.uppercase()) }
                        .getOrElse { SubtitleStyle.assModeOf(value) }
                } else SubtitleStyle.assModeOf(value)
                native.setProperty("sub-ass-override", mode.mpvValue).also { written ->
                    if (written) SubtitleStyleStore(this).update { it.copy(assMode = mode) }
                }
            }
            name == "sub-border-style" -> {
                val style = SubtitleBorderStyle.entries.firstOrNull {
                    it.mpvValue == value.lowercase() || it.name == value.uppercase()
                }
                if (style == null) false else native.setProperty(name, style.mpvValue).also { written ->
                    if (written) SubtitleStyleStore(this).update { it.copy(borderStyle = style) }
                }
            }
            name == "sub-codepage" ->
                // demuxer 局部选项：裸 `sub-codepage` 不是属性（setProperty 会 ok=false）；
                // 运行时只有 `options/<name>` 命名空间可写（实测 ok=true）。
                // ⚠️ 但**写入不等于生效**：external 字幕的 demuxer 在创建时读该默认值，
                // 已加载的字幕不会重解；mpv 也没有 `sub-reload` 命令（cmd_list.c 里没有）。
                // 真正生效要"移除外挂轨 + 重新 add"或把它作为**初始化选项**（W3-R1 范围）。
                native.setProperty("options/sub-codepage", value)
            else -> false
        }
        smbTrace("probe sub-write $name=$value ok=$ok")
    }
    panelRevision++
}
