package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.mpv.MpvTrackCatalog
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.playback.subtitle.SubtitleStyle
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.player.PlayerPanelData
import com.kivo.cinema.ui.compose.player.PlayerPanelForms
import com.kivo.cinema.ui.compose.player.PlayerRowControl
import com.kivo.cinema.ui.compose.readProperty
import kotlin.math.roundToInt

/**
 * 字幕面板的**行数据装配**（P9 从 `PlayerPanelData` / `PlayerPanelItems` 迁出）。
 *
 * 【为什么单独一个文件】三张面板（入口 / 显示设置 / 主次轨道）的字段在 P9 里
 * 从 5 个涨到 16 个，塞回 224 行的行模型文件会把"13 种形态共用一个行模型"
 * 的收口撑破；这里只长字幕这一块，行模型本身保持不动。
 *
 * 【仍然只装配，不取数】所有值来自调用方传入的内核真值；读不到就 `notReady`，
 * 绝不编默认值（铁律）。
 *
 * 【P10 只改了两件事】行首装饰图标去掉；显示设置从 6 段散组收成 **7 个具名组**
 * （ASS 样式 / 字号预设 / 微调 / 颜色与边框 / 效果 / 同步 / 次字幕）。
 * **行顺序、tag、内核属性、FONT_PRESETS、步进区间、主次字幕互斥一律不动**。
 */
internal object SubtitlePanelRows {

    private const val ASS_SECTION = "ASS 样式"
    private const val PRESET_SECTION = "字号预设"
    private const val TUNE_SECTION = "微调"
    private const val COLOR_SECTION = "颜色与边框"
    private const val EFFECT_SECTION = "效果"
    private const val SYNC_SECTION = "同步"
    private const val SECONDARY_SECTION = "次字幕"

    /** 字幕面板入口：内嵌 / 外挂 / 次字幕 + 搜索导入显示关闭。 */
    fun entry(mpv: MpvPlaybackSession): List<PlayerOptionItem> {
        val catalog = mpv.tracksOrNull() ?: return PlayerPanelForms.notReady()
        val tracks = catalog.subtitleTracks()
        val mainId = catalog.currentTrackId("sub")
        val current = tracks.firstOrNull { it.id == mainId }
        val secondaryId = mpv.readProperty("secondary-sid")?.trim()?.toIntOrNull()
        val secondary = tracks.firstOrNull { it.id == secondaryId }
        val embedded = current?.takeIf { !it.external }?.let { SubtitleTrackLabels.title(it) } ?: "关闭"
        val external = current?.takeIf { it.external }?.let { SubtitleTrackLabels.title(it) } ?: "未导入"
        return listOf(
            PlayerOptionItem(
                title = "内嵌字幕",
                subtitle = embedded,
                tag = "subtitleTracks",
                control = PlayerRowControl.CHEVRON,
            ),
            PlayerOptionItem(
                title = "外挂字幕",
                subtitle = external,
                tag = "subtitleExternal",
                control = PlayerRowControl.CHEVRON,
            ),
            PlayerOptionItem(
                title = "次字幕",
                subtitle = secondary?.let { SubtitleTrackLabels.title(it) } ?: "关闭",
                tag = "subtitleSecondary",
                control = PlayerRowControl.CHEVRON,
            ),
        ) + withAiRow(withEncodingRow(PlayerPanelForms.subtitleActions()))
    }

    /** 「AI 中文字幕」行紧跟「字幕编码」（v1 静态行；状态投影走点击反馈，见文件头偏差登记）。
     *  先移除调用方已带的 sub-ai 再插入：subtitleActions() 与本函数是两个来源，
     *  不去重会得到相邻两行同 tag 行（2026-11-22 设置页接线时收敛）。 */
    private fun withAiRow(actions: List<PlayerOptionItem>): List<PlayerOptionItem> {
        val withoutAi = actions.filter { it.tag != "sub-ai" }
        val row = PlayerOptionItem(title = "AI 中文字幕", tag = "sub-ai")
        val index = withoutAi.indexOfFirst { it.tag == "subtitleEncoding" }
        return if (index < 0) withoutAi + row else withoutAi.toMutableList().apply { add(index + 1, row) }
    }

    /** 「字幕编码」钻入行紧跟「粘贴字幕链接」（R1 入口）。 */
    private fun withEncodingRow(actions: List<PlayerOptionItem>): List<PlayerOptionItem> {
        val row = PlayerOptionItem(
            title = "字幕编码",
            tag = "subtitleEncoding",
            control = PlayerRowControl.CHEVRON,
        )
        val index = actions.indexOfFirst { it.tag == "sub-import-url" }
        return if (index < 0) actions + row else actions.toMutableList().apply { add(index + 1, row) }
    }

    /**
     * 显示设置卡（形态 9）：7 个具名组。
     *
     * 【文案边界】除「字幕延迟 / 次字幕位置」外，全部**只对非 ASS 文本字幕生效**
     * 或只在「统一样式」下生效 —— 分组头已写明，行内不再重复。
     */
    fun display(mpv: MpvPlaybackSession): List<PlayerOptionItem> {
        val font = mpv.readProperty("sub-font-size")?.toDoubleOrNull()
        val pos = mpv.readProperty("sub-pos")?.toDoubleOrNull()
        val marginY = mpv.readProperty("sub-margin-y")?.toDoubleOrNull()
        val border = mpv.readProperty("sub-border-size")?.toDoubleOrNull()
        val shadow = mpv.readProperty("sub-shadow-offset")?.toDoubleOrNull()
        val spacing = mpv.readProperty("sub-spacing")?.toDoubleOrNull()
        val delay = mpv.readProperty("sub-delay")?.toDoubleOrNull()
        val secondaryPos = mpv.readProperty("secondary-sub-pos")?.toDoubleOrNull()
        // SUB-M5：次字幕延迟也是内核真值，读不到就整卡 notReady（不编默认值）。
        val secondaryDelay = mpv.readProperty("secondary-sub-delay")?.toDoubleOrNull()
        if (font == null || pos == null || marginY == null || border == null || shadow == null ||
            spacing == null || delay == null || secondaryPos == null || secondaryDelay == null
        ) {
            return PlayerPanelForms.notReady()
        }
        val color = mpv.readProperty("sub-color").orEmpty()
        val outline = mpv.readProperty("sub-outline-color").orEmpty()
        val back = mpv.readProperty("sub-back-color").orEmpty()
        val borderStyle = borderStyleLabel(mpv.readProperty("sub-border-style").orEmpty())
        val assMode = SubtitleStyle.assModeOf(mpv.readProperty("sub-ass-override"))
        val bold = mpv.readProperty("sub-bold") == "yes"
        val secondaryVisible = mpv.readProperty("secondary-sub-visibility") != "no"

        val rows = mutableListOf<PlayerOptionItem>()
        rows += PlayerOptionItem(
            title = "ASS 字幕样式",
            value = assMode.label,
            subtitle = "统一样式会覆盖源排版",
            tag = "sub-ass-mode",
            section = ASS_SECTION,
        )
        SubtitleStyle.FONT_PRESETS.forEach { preset ->
            rows += PlayerOptionItem(
                title = "字号 ${preset.label}",
                value = preset.size.toString(),
                selected = font.roundToInt() == preset.size,
                tag = "sub-preset-${preset.size}",
                section = PRESET_SECTION,
                control = PlayerRowControl.CHECK,
            )
        }
        rows += stepper("字号", "sub-font-size", font, "", 0, TUNE_SECTION)
        rows += stepper("垂直位置", "sub-pos", pos, "%", 0, TUNE_SECTION)
        rows += stepper("底部边距", "sub-margin-y", marginY, "", 0, TUNE_SECTION, subtitle = "仅文本字幕")
        rows += stepper("描边粗细", "sub-border-size", border, "", 1, TUNE_SECTION)
        rows += cycleRow("文字颜色", "sub-color", SubtitleStyle.colorLabel(SubtitleStyle.COLORS, color), COLOR_SECTION)
        rows += cycleRow(
            "描边颜色",
            "sub-outline-color",
            SubtitleStyle.colorLabel(SubtitleStyle.OUTLINE_COLORS, outline),
            COLOR_SECTION,
        )
        rows += cycleRow(
            "背景框颜色",
            "sub-back-color",
            SubtitleStyle.colorLabel(SubtitleStyle.BACK_COLORS, back),
            COLOR_SECTION,
        )
        rows += cycleRow("边框样式", "sub-border-style", borderStyle, COLOR_SECTION)
        rows += stepper("阴影偏移", "sub-shadow-offset", shadow, "", 1, EFFECT_SECTION)
        rows += stepper("字间距", "sub-spacing", spacing, "", 1, EFFECT_SECTION)
        rows += PlayerOptionItem(
            title = "加粗",
            subtitle = "仅文本字幕",
            tag = "sub-bold",
            control = PlayerRowControl.TOGGLE,
            on = bold,
            section = EFFECT_SECTION,
        )
        rows += stepper("字幕延迟", "sub-delay", delay, " s", 1, SYNC_SECTION)
        rows += stepper("次字幕位置", "secondary-sub-pos", secondaryPos, "%", 0, SECONDARY_SECTION)
        // SUB-M5：次字幕自己的延迟轴（按片源记忆，与主延迟互不覆盖）。
        rows += stepper("次字幕延迟", "secondary-sub-delay", secondaryDelay, " s", 1, SECONDARY_SECTION)
        rows += PlayerOptionItem(
            title = "显示次字幕",
            tag = "secondary-sub-visibility",
            control = PlayerRowControl.TOGGLE,
            on = secondaryVisible,
            section = SECONDARY_SECTION,
        )
        return rows
    }

    /** 内嵌字幕（P9 · G：首行「关闭字幕」+ 强制/听障徽章）。 */
    fun embedded(
        tracks: List<MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> = listOf(closeRow(currentId, section = "切换")) +
        tracks.map { trackRow(it, currentId, includeExternal = false) }

    /** 外挂字幕（P9 · H：长按行删除；标题来自导入时的原文件名）。 */
    fun external(
        tracks: List<MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> = tracks.map {
        trackRow(it, currentId, includeExternal = true).copy(section = "外挂")
    }

    /** 次字幕列表（P9 · F）：关闭 + 可选的轨；主字幕那条置灰并说明原因。 */
    fun secondary(
        tracks: List<MpvTrackCatalog.Track>,
        secondaryId: Int?,
        mainId: Int,
    ): List<PlayerOptionItem> {
        val close = PlayerOptionItem(
            title = "关闭次字幕",
            subtitle = if (secondaryId == null) "当前未启用" else "只保留主字幕",
            selected = secondaryId == null,
            tag = "secondary:no",
            section = "切换",
            control = PlayerRowControl.CHECK,
        )
        val rows = tracks.map { track ->
            val same = track.id == mainId
            PlayerOptionItem(
                title = SubtitleTrackLabels.title(track),
                subtitle = SubtitleTrackLabels.subtitle(track, includeExternal = true) +
                    if (same) " · 不能与主字幕相同" else "",
                selected = track.id == secondaryId,
                enabled = !same,
                tag = "secondary:${track.id}",
                section = "切换",
                control = PlayerRowControl.CHECK,
            )
        }
        return listOf(close) + rows
    }

    private fun trackRow(
        track: MpvTrackCatalog.Track,
        currentId: Int,
        includeExternal: Boolean,
    ): PlayerOptionItem = PlayerOptionItem(
        title = SubtitleTrackLabels.title(track),
        subtitle = SubtitleTrackLabels.subtitle(track, includeExternal),
        selected = track.id == currentId,
        tag = track.id.toString(),
        section = "切换",
        control = PlayerRowControl.CHECK,
    )

    private fun closeRow(currentId: Int, section: String) = PlayerOptionItem(
        title = "关闭字幕",
        subtitle = "不显示任何字幕",
        selected = currentId < 0,
        tag = "sub-off",
        section = section,
    )

    private fun stepper(
        title: String,
        tag: String,
        value: Double,
        unit: String,
        digits: Int,
        section: String,
        subtitle: String = "",
    ): PlayerOptionItem = PlayerPanelData.stepper(title, tag, value, unit, digits)
        .copy(section = section, subtitle = subtitle)

    private fun cycleRow(title: String, tag: String, value: String, section: String): PlayerOptionItem =
        PlayerOptionItem(title = title, value = value, tag = tag, section = section)

    private fun borderStyleLabel(raw: String): String = when (raw.trim().lowercase()) {
        "opaque-box" -> "不透明框"
        "background-box" -> "背景框"
        "outline-and-shadow" -> "描边阴影"
        else -> "描边阴影"
    }
}