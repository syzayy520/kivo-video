package com.kivo.cinema.ui.compose.player

import com.kivo.cinema.playback.MpvPlaybackSession

/**
 * 面板的**行模型 + 内核真值装配**（v3 §1.4，纯装配，不取数）。
 *
 * 【为什么 13 种形态共用一个行模型】REX 的 13 个面板拆开看只有四种壳，
 * 而四种壳里的"一行"长得都一样：图标 / 题 / 副题 / 值 / 选中态 / 控件。
 * 每加一个面板就写一个面板文件，正是"为了凑文件而凑"的反面 ——
 * **十三个长得一样的东西，应该是同一个东西的十三份数据。**
 *
 * 【为什么取数不在这里】读 mpv 属性要抢核心锁（11 条轨道 = 99 次 JNI），
 * 必须在 IO 线程做 ⇒ 取数时机归 [rememberPanelItems]，这里只把值装成行。
 */
internal object PlayerPanelData {

    val SPEEDS = listOf("0.5x", "0.75x", "1x", "1.25x", "1.5x", "1.75x", "2x")

    val QUALITIES = listOf("16:9", "4:3", "2.35:1", "1.85:1", "2.39:1")

    /** 倍速列表。current=null（读不到）⇒ **不预选中任何档**，不撒谎。 */
    fun speed(current: String?): List<PlayerOptionItem> = SPEEDS.map {
        PlayerOptionItem(
            title = it,
            selected = it == current,
            tag = it,
            control = PlayerRowControl.CHECK,
        )
    }

    /**
     * 画质比例列表。tag 直接是能写进 `video-aspect-override` 的串。
     *
     * 【为什么还要比 "Original"/"-1"】mpv 默认 -1（原始比例），读回来可能是
     * "Original" 也可能是 "-1.000"。只判空会让"自动"永远没有勾。
     */
    fun qualities(current: String): List<PlayerOptionItem> {
        val auto = current.isBlank() ||
            current.equals("Original", ignoreCase = true) ||
            current.trim().startsWith("-")
        return listOf(
            PlayerOptionItem(
                title = "自动",
                selected = auto,
                tag = "Original",
                section = "比例",
                control = PlayerRowControl.CHECK,
            ),
        ) + QUALITIES.map {
            PlayerOptionItem(
                title = it,
                selected = it == current,
                tag = it,
                section = "比例",
                control = PlayerRowControl.CHECK,
            )
        }
    }

    /** 音轨列表（形态 4：单音轨时就是一行 ✓aac + 一行钻入延迟）。 */
    fun audioTracks(
        tracks: List<com.kivo.cinema.mpv.MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> = tracks.map {
        PlayerOptionItem(
            title = it.displayName(it.id),
            subtitle = it.codec,
            selected = it.id == currentId,
            tag = it.id.toString(),
            control = PlayerRowControl.CHECK,
        )
    }

    /** 内封字幕列表（形态 1 的"切换"组，带 icon、双行 60pt）。 */
    fun subtitleTracks(
        tracks: List<com.kivo.cinema.mpv.MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> = tracks.map {
        PlayerOptionItem(
            title = it.displayName(it.id),
            subtitle = it.codec,
            selected = it.id == currentId,
            tag = it.id.toString(),
            icon = "captions",
            section = "切换",
            control = PlayerRowControl.CHECK,
        )
    }

    /** 音频延迟微调卡（形态 5）。 */
    fun audioDelay(seconds: Double): List<PlayerOptionItem> = listOf(
        stepper("音频延迟", "audio-delay", seconds, " s", 1),
    )

    /**
     * 字幕显示设置微调卡（§1.6：形态 9 同壳）。
     * toggle＝标准统一样式（sub-ass-override=yes）；关＝保留 ASS 源特效（no）。
     * 四个 stepper 全是 mpv sub-* 真值，写后 revision++ 重读。
     */
    fun subtitleDisplay(
        fontSize: Double,
        pos: Double,
        delay: Double,
        border: Double,
        standardOn: Boolean,
    ): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "标准统一样式",
            subtitle = "关闭后保留 ASS 源特效",
            tag = "sub-ass-override",
            control = PlayerRowControl.TOGGLE,
            on = standardOn,
        ),
        stepper("字号", "sub-font-size", fontSize, "", 0),
        stepper("垂直位置", "sub-pos", pos, "%", 0),
        stepper("字幕延迟", "sub-delay", delay, " s", 1),
        stepper("描边", "sub-border-size", border, "", 1),
    )

    /**
     * 弹幕显示设置微调卡（形态 9，v3.1）。
     * 三个 stepper 写本地态（弹幕是 Compose 自绘层，mpv 没有这些属性）；
     * 区域三行是单选。只有渲染器真支持的项才给控件。
     */
    fun danmakuSettings(settings: com.kivo.cinema.danmaku.DanmakuSettings): List<PlayerOptionItem> = listOf(
        stepper("滚动时长", "danmaku-speed", settings.speedSec.toDouble(), " s", 1),
        stepper("字号", "danmaku-font", settings.fontScale.toDouble(), "×", 1),
        stepper("不透明度", "danmaku-opacity", (settings.opacity * 100).toDouble(), "%", 0),
        areaRow("全屏", "danmaku-area-full", settings.area == com.kivo.cinema.danmaku.DanmakuArea.FULL),
        areaRow("上半屏", "danmaku-area-top", settings.area == com.kivo.cinema.danmaku.DanmakuArea.TOP_HALF),
        areaRow("下半屏", "danmaku-area-bottom", settings.area == com.kivo.cinema.danmaku.DanmakuArea.BOTTOM_HALF),
    )

    private fun areaRow(title: String, tag: String, selected: Boolean) = PlayerOptionItem(
        title = title,
        selected = selected,
        tag = tag,
        section = "显示区域",
        control = PlayerRowControl.CHECK,
    )

    /**
     * 资源内联展开（形态 11）：chips 组 + 源卡组。
     *
     * 【为什么 chips 只有一项】chips 是"同一部片的不同版本"，
     * 我们的资源库还没有多版本 ⇒ 只列**内核真报的那一个**（SDR / HDR10）。
     * 编出"1080P/4K"两个 chip 让用户点了没反应，比少一个 chip 更糟。
     */
    fun resource(badge: String, meta: String, source: String, chip: String): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = chip, selected = true, tag = chip, section = "chips"),
        PlayerOptionItem(
            title = badge,
            subtitle = meta,
            value = source,
            tag = source,
            section = "source",
        ),
    )

    /** 一行微调（−/+ pill）。tag＝mpv 属性名，step＝当前值 ⇒ 路由直接写回。 */
    fun stepper(
        title: String,
        tag: String,
        value: Double,
        unit: String,
        digits: Int,
    ): PlayerOptionItem = PlayerOptionItem(
        title = title,
        value = String.format("%.${digits}f%s", value, unit),
        tag = tag,
        step = value,
        control = PlayerRowControl.STEPPER,
    )

    /** 档位串 → mpv 数值；不合法返回 null（**不猜**）。 */
    fun speedValue(label: String): Double? {
        val value = label.removeSuffix("x").toDoubleOrNull() ?: return null
        return if (value in 0.1..10.0) value else null
    }

    /**
     * 从内核读当前倍速。读不到返回 **null** 而不是编一个 "1x" ——
     * 面板可能在起播前被打开，报"1x"等于撒谎。
     */
    fun currentSpeedLabel(session: MpvPlaybackSession?): String? {
        val speed = session?.currentSpeedOrNull() ?: return null
        val text = if (speed % 1.0 == 0.0) speed.toInt().toString() else speed.toString()
        return "${text}x"
    }
}

/** 行尾控件（四种壳共用）。 */
internal enum class PlayerRowControl { NONE, CHECK, CHEVRON, STEPPER, TOGGLE }

/**
 * 面板的一行。
 *
 * @param title 行标题（资源卡里＝SDR/HDR 徽章）。
 * @param value 右侧的值（微调行＝当前数值；资源卡＝源名）。
 * @param subtitle 标题下的补充说明。
 * @param selected 是否选中（列表＝白勾，chips＝白描边）。
 * @param tag 回调带回去的标识：mpv 属性名 / 轨道 id / 目标面板 kind。
 * @param icon 行首图标（[PlayerGlyph] 的 kind）；null＝无图标，选中勾落在行首。
 * @param section 分组头；同组连续行只画一次头。
 * @param control 行尾控件。
 * @param enabled false＝**能力缺失**，必须同时给非空 subtitle 说明原因（铁律）。
 * @param step 微调行的当前数值。
 * @param on 开关行的当前状态。
 */
internal data class PlayerOptionItem(
    val title: String,
    val value: String = "",
    val subtitle: String = "",
    val selected: Boolean = false,
    val tag: String = "",
    val icon: String? = null,
    val section: String = "",
    val control: PlayerRowControl = PlayerRowControl.NONE,
    val enabled: Boolean = true,
    val step: Double = 0.0,
    val on: Boolean = false,
)