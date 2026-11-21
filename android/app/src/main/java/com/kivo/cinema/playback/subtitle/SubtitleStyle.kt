package com.kivo.cinema.playback.subtitle

import kotlin.math.roundToInt

/**
 * 字幕样式模型（P9 · A/B/C/E/F 的共同真值）。
 *
 * 【为什么是一个不可变数据类而不是散落的 prefs 读取】
 * 样式有 12 个字段，起播时要一次性翻成 mpv 属性、面板要逐项改并落盘、
 * 设置页要读写同一个 ASS 模式。三处若各自拼属性名/默认值，迟早像
 * `52 vs 55` 那样分叉。[properties] 是**唯一**的属性表。
 *
 * 【单位】`sub-font-size` / `sub-margin-y` 是 scaled pixel（mpv 以 720 高为基准
 * 缩放）⇒ 40 在 1080 高画面上约 60 px ≈ 5.6%，即本波的目标带。
 * `sub-pos` 0–100（>100 会被 libass 裁掉，不是"更高"）。
 * 颜色一律 `#AARRGGBB`（mpv 的 print 格式，读写同构）。
 */
internal data class SubtitleStyle(
    val fontSize: Int = DEFAULT_FONT_SIZE,
    val pos: Int = DEFAULT_POS,
    val marginY: Int = DEFAULT_MARGIN_Y,
    val borderSize: Double = DEFAULT_BORDER_SIZE,
    val color: String = DEFAULT_COLOR,
    val outlineColor: String = DEFAULT_OUTLINE_COLOR,
    val backColor: String = DEFAULT_BACK_COLOR,
    val borderStyle: SubtitleBorderStyle = SubtitleBorderStyle.OUTLINE,
    val shadowOffset: Double = DEFAULT_SHADOW_OFFSET,
    val spacing: Double = DEFAULT_SPACING,
    val bold: Boolean = false,
    val assMode: SubtitleAssMode = SubtitleAssMode.SOURCE,
    val secondaryPos: Int = DEFAULT_SECONDARY_POS,
    val secondaryVisible: Boolean = true,
) {

    /** mpv 属性表（顺序 = 下发顺序）。**唯一**权威列表。 */
    fun properties(): List<Pair<String, String>> = listOf(
        "sub-font-size" to fontSize.toString(),
        "sub-pos" to pos.toString(),
        "sub-margin-y" to marginY.toString(),
        "sub-border-size" to format(borderSize),
        "sub-color" to color,
        "sub-outline-color" to outlineColor,
        "sub-back-color" to backColor,
        "sub-border-style" to borderStyle.mpvValue,
        "sub-shadow-offset" to format(shadowOffset),
        "sub-spacing" to format(spacing),
        "sub-bold" to if (bold) "yes" else "no",
        "sub-ass-override" to assMode.mpvValue,
        "secondary-sub-pos" to secondaryPos.toString(),
        "secondary-sub-visibility" to if (secondaryVisible) "yes" else "no",
    )

    /** 数值字段统一夹取（面板步进与 prefs 读入共用，防手改 prefs 越界）。 */
    fun sanitized(): SubtitleStyle = copy(
        fontSize = clampFontSize(fontSize.toDouble()),
        pos = clampPos(pos.toDouble()),
        marginY = clampMarginY(marginY.toDouble()),
        borderSize = clampBorderSize(borderSize),
        color = normalizeColor(color, DEFAULT_COLOR),
        outlineColor = normalizeColor(outlineColor, DEFAULT_OUTLINE_COLOR),
        backColor = normalizeColor(backColor, DEFAULT_BACK_COLOR),
        shadowOffset = clampShadowOffset(shadowOffset),
        spacing = clampSpacing(spacing),
        secondaryPos = clampPos(secondaryPos.toDouble()),
    )

    companion object {
        /**
         * 默认字号：mpv scaled pixel。40 @1080 ≈ 60 px ≈ 5.6% 屏高。
         *
         * 【为什么不再用 52】52 在 1080p 上是 78 px（≈7.2%），中文双行时
         * 会盖住画面主体；40 与主流流媒体（≈5%）同量级。
         * 三处必须同源：C++ 初值 / 本类默认 / 面板重置。
         */
        const val DEFAULT_FONT_SIZE = 40

        /** 默认垂直位置：mpv 的 100 = 底部基线（0 = 顶部）。 */
        const val DEFAULT_POS = 100

        /**
         * 默认底距：40 scaled pixel ≈ 60 px @1080。
         *
         * 【只影响非 ASS 文本字幕】mpv 手册：`sub-margin-y` 是
         * "vertical margins of unstyled text subtitles"；ASS 的位置由源样式
         * 或 `sub-pos` 决定。面板文案必须写明"仅文本字幕"。
         */
        const val DEFAULT_MARGIN_Y = 40

        const val DEFAULT_BORDER_SIZE = 3.0
        const val DEFAULT_SHADOW_OFFSET = 0.0
        const val DEFAULT_SPACING = 0.0
        const val DEFAULT_SECONDARY_POS = 15
        const val DEFAULT_COLOR = "#FFFFFFFF"
        const val DEFAULT_OUTLINE_COLOR = "#FF000000"
        const val DEFAULT_BACK_COLOR = "#99000000"

        const val MIN_FONT = 20
        const val MAX_FONT = 120
        const val MIN_POS = 0
        const val MAX_POS = 100
        const val MIN_MARGIN_Y = 0
        const val MAX_MARGIN_Y = 300
        const val MAX_BORDER = 10.0
        const val MAX_SHADOW = 10.0
        const val MIN_SPACING = -10.0
        const val MAX_SPACING = 10.0
        const val MAX_DELAY_SECONDS = 30.0

        val COLORS: List<ColorOption> = listOf(
            ColorOption("#FFFFFFFF", "白色"),
            ColorOption("#FFFFF1D0", "暖白"),
            ColorOption("#FFFFD400", "黄色"),
            ColorOption("#FF8DFF9A", "绿色"),
            ColorOption("#FF7DE8FF", "青色"),
            ColorOption("#FFFF9EC4", "粉色"),
        )

        val OUTLINE_COLORS: List<ColorOption> = listOf(
            ColorOption("#FF000000", "黑色"),
            ColorOption("#FF3A3A3A", "深灰"),
            ColorOption("#FFFFFFFF", "白色"),
        )

        val BACK_COLORS: List<ColorOption> = listOf(
            ColorOption("#00000000", "无背景"),
            ColorOption("#99000000", "半透明黑"),
            ColorOption("#E6000000", "深黑"),
        )

        /** 字号预设（小/中/大）：只改字号，不碰其余样式。 */
        val FONT_PRESETS: List<FontPreset> = listOf(
            FontPreset("小", 32),
            FontPreset("中", DEFAULT_FONT_SIZE),
            FontPreset("大", 48),
        )

        fun clampFontSize(value: Double): Int = value.roundToInt().coerceIn(MIN_FONT, MAX_FONT)

        fun clampPos(value: Double): Int = value.roundToInt().coerceIn(MIN_POS, MAX_POS)

        fun clampMarginY(value: Double): Int = value.roundToInt().coerceIn(MIN_MARGIN_Y, MAX_MARGIN_Y)

        fun clampBorderSize(value: Double): Double = value.coerceIn(0.0, MAX_BORDER)

        fun clampShadowOffset(value: Double): Double = value.coerceIn(0.0, MAX_SHADOW)

        fun clampSpacing(value: Double): Double = value.coerceIn(MIN_SPACING, MAX_SPACING)

        fun clampDelay(value: Double): Double = value.coerceIn(-MAX_DELAY_SECONDS, MAX_DELAY_SECONDS)

        /** 颜色环：当前值不在调色板里（手改过 prefs）⇒ 从第一个开始，不猜。 */
        fun cycleColor(palette: List<ColorOption>, current: String): ColorOption {
            val index = palette.indexOfFirst { it.value.equals(current, ignoreCase = true) }
            return palette[(index + 1).mod(palette.size)]
        }

        fun colorLabel(palette: List<ColorOption>, current: String): String =
            palette.firstOrNull { it.value.equals(current, ignoreCase = true) }?.label ?: "自定义"

        fun assModeNext(current: SubtitleAssMode): SubtitleAssMode = when (current) {
            SubtitleAssMode.SOURCE -> SubtitleAssMode.SCALE
            SubtitleAssMode.SCALE -> SubtitleAssMode.FORCE
            SubtitleAssMode.FORCE -> SubtitleAssMode.SOURCE
        }

        /** 内核回读值 → 枚举。`strip`（mpv 次字幕默认）不是三态之一，按"剥离"显示。 */
        fun assModeOf(mpvValue: String?): SubtitleAssMode = when (mpvValue?.trim()?.lowercase()) {
            "scale" -> SubtitleAssMode.SCALE
            "force" -> SubtitleAssMode.FORCE
            "strip" -> SubtitleAssMode.FORCE
            else -> SubtitleAssMode.SOURCE
        }

        fun normalizeColor(value: String, fallback: String): String {
            val v = value.trim()
            val ok = (v.length == 7 || v.length == 9) && v.startsWith("#") &&
                v.drop(1).all { it.isDigit() || it.lowercaseChar() in 'a'..'f' }
            return if (ok) v.uppercase() else fallback
        }

        private fun format(value: Double): String = String.format(java.util.Locale.US, "%.1f", value)
    }
}

/** mpv `sub-ass-override` 的三态（C++/属性值见字段）。 */
internal enum class SubtitleAssMode(val mpvValue: String, val label: String) {
    SOURCE("no", "源样式"),
    SCALE("scale", "仅缩放"),
    FORCE("force", "统一样式"),
}

/** mpv `sub-border-style`。 */
internal enum class SubtitleBorderStyle(val mpvValue: String, val label: String) {
    OUTLINE("outline-and-shadow", "描边阴影"),
    OPAQUE_BOX("opaque-box", "不透明框"),
    BACKGROUND_BOX("background-box", "背景框"),
}

internal data class ColorOption(val value: String, val label: String)

internal data class FontPreset(val label: String, val size: Int)
