package com.kivo.cinema.playback.subtitle

import android.content.Context

/**
 * 字幕全局样式的持久化（P9 · C）。持久化域 = 播放策略域，UI 只编辑。
 *
 * 【为什么单独一个 prefs 文件】样式与 PlaybackPrefs（缓冲/手势/语言）是两种
 * 变化原因：播放器几乎每个会话都读样式，而缓冲只在起播读一次；
 * 共用一个键空间会让"改样式"和"改缓冲"在同一份 XML 里互相踩。
 *
 * 【旧键迁移】262 的「忽略特效字幕样式」是 bool（`playback_prefs.ignore_ass_style`），
 * 现在 ASS 模式是三态。首次读取且本文件还没有 `ass_mode` 时：true ⇒ 统一样式，
 * false ⇒ 源样式，并立刻写入新键；旧键保留（回滚安全）但不再被读取。
 */
internal class SubtitleStyleStore(context: Context) {

    private val app = context.applicationContext
    private val sp = app.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** 读样式并做一次区间夹取（手改 prefs 也不能把 sub-pos 写成 150）。 */
    fun load(): SubtitleStyle {
        migrateLegacyAssMode()
        return SubtitleStyle(
            fontSize = sp.getInt(KEY_FONT_SIZE, SubtitleStyle.DEFAULT_FONT_SIZE),
            pos = sp.getInt(KEY_POS, SubtitleStyle.DEFAULT_POS),
            marginY = sp.getInt(KEY_MARGIN_Y, SubtitleStyle.DEFAULT_MARGIN_Y),
            borderSize = sp.getFloat(KEY_BORDER_SIZE, SubtitleStyle.DEFAULT_BORDER_SIZE.toFloat()).toDouble(),
            color = sp.getString(KEY_COLOR, SubtitleStyle.DEFAULT_COLOR) ?: SubtitleStyle.DEFAULT_COLOR,
            outlineColor = sp.getString(KEY_OUTLINE_COLOR, SubtitleStyle.DEFAULT_OUTLINE_COLOR)
                ?: SubtitleStyle.DEFAULT_OUTLINE_COLOR,
            backColor = sp.getString(KEY_BACK_COLOR, SubtitleStyle.DEFAULT_BACK_COLOR)
                ?: SubtitleStyle.DEFAULT_BACK_COLOR,
            borderStyle = borderStyleOf(sp.getString(KEY_BORDER_STYLE, null)),
            shadowOffset = sp.getFloat(KEY_SHADOW, SubtitleStyle.DEFAULT_SHADOW_OFFSET.toFloat()).toDouble(),
            spacing = sp.getFloat(KEY_SPACING, SubtitleStyle.DEFAULT_SPACING.toFloat()).toDouble(),
            bold = sp.getBoolean(KEY_BOLD, false),
            // 落盘存枚举名（SOURCE/SCALE/FORCE）；同时兼容有人手改成 mpv 值。
            assMode = assModeOf(sp.getString(KEY_ASS_MODE, null)),
            secondaryPos = sp.getInt(KEY_SECONDARY_POS, SubtitleStyle.DEFAULT_SECONDARY_POS),
            secondaryVisible = sp.getBoolean(KEY_SECONDARY_VISIBLE, true),
        ).sanitized()
    }

    fun save(style: SubtitleStyle) {
        synchronized(ioLock) { write(style) }
    }

    /** 读-改-写：[transform] 拿到当前落盘值，返回新值。 */
    fun update(transform: (SubtitleStyle) -> SubtitleStyle): SubtitleStyle = synchronized(ioLock) {
        val next = transform(load()).sanitized()
        write(next)
        next
    }

    /** 面板重置：回到与 C++ 初值/文档一致的出厂样式。 */
    fun reset(): SubtitleStyle = synchronized(ioLock) {
        val style = SubtitleStyle()
        write(style)
        style
    }

    private fun write(style: SubtitleStyle) {
        val s = style.sanitized()
        sp.edit()
            .putInt(KEY_FONT_SIZE, s.fontSize)
            .putInt(KEY_POS, s.pos)
            .putInt(KEY_MARGIN_Y, s.marginY)
            .putFloat(KEY_BORDER_SIZE, s.borderSize.toFloat())
            .putString(KEY_COLOR, s.color)
            .putString(KEY_OUTLINE_COLOR, s.outlineColor)
            .putString(KEY_BACK_COLOR, s.backColor)
            .putString(KEY_BORDER_STYLE, s.borderStyle.name)
            .putFloat(KEY_SHADOW, s.shadowOffset.toFloat())
            .putFloat(KEY_SPACING, s.spacing.toFloat())
            .putBoolean(KEY_BOLD, s.bold)
            .putString(KEY_ASS_MODE, s.assMode.name)
            .putInt(KEY_SECONDARY_POS, s.secondaryPos)
            .putBoolean(KEY_SECONDARY_VISIBLE, s.secondaryVisible)
            .apply()
    }

    private fun migrateLegacyAssMode() {
        if (sp.contains(KEY_ASS_MODE)) return
        // 直读旧键：不与 PlaybackPrefs 产生"两个类互相构造"的环。
        val legacy = app.getSharedPreferences(LEGACY_PREFS, Context.MODE_PRIVATE)
            .getBoolean(LEGACY_IGNORE_ASS, false)
        val mode = if (legacy) SubtitleAssMode.FORCE else SubtitleAssMode.SOURCE
        sp.edit().putString(KEY_ASS_MODE, mode.name).apply()
    }

    private fun borderStyleOf(raw: String?): SubtitleBorderStyle =
        SubtitleBorderStyle.entries.firstOrNull { it.name == raw } ?: SubtitleBorderStyle.OUTLINE

    private fun assModeOf(raw: String?): SubtitleAssMode =
        SubtitleAssMode.entries.firstOrNull { it.name == raw } ?: SubtitleStyle.assModeOf(raw)

    companion object {
        const val PREFS = "subtitle_prefs"

        /**
         * 读改写互斥锁。**必须是类级的**：调用方每次 `SubtitleStyleStore(context)` 新建实例，
         * 实例锁等于没有锁（并发步进各自 load→改→save 会丢更新）。
         */
        private val ioLock = Any()

        private const val LEGACY_PREFS = "playback_prefs"
        private const val LEGACY_IGNORE_ASS = "ignore_ass_style"

        private const val KEY_FONT_SIZE = "font_size"
        private const val KEY_POS = "pos"
        private const val KEY_MARGIN_Y = "margin_y"
        private const val KEY_BORDER_SIZE = "border_size"
        private const val KEY_COLOR = "color"
        private const val KEY_OUTLINE_COLOR = "outline_color"
        private const val KEY_BACK_COLOR = "back_color"
        private const val KEY_BORDER_STYLE = "border_style"
        private const val KEY_SHADOW = "shadow_offset"
        private const val KEY_SPACING = "spacing"
        private const val KEY_BOLD = "bold"
        private const val KEY_ASS_MODE = "ass_mode"
        private const val KEY_SECONDARY_POS = "secondary_pos"
        private const val KEY_SECONDARY_VISIBLE = "secondary_visible"
    }
}
