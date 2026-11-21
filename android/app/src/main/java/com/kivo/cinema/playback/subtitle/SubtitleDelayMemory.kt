package com.kivo.cinema.playback.subtitle

import android.content.Context

/**
 * **按片源**记忆字幕延迟（P9 · C）。
 *
 * 【为什么延迟不能进全局样式】延迟是"这个片源的字幕轴偏了几百毫秒"，
 * 属于片源属性；写进全局样式会让下一部片的字幕整体错位。
 * 键来自 [SubtitleSourceKey]（URL / SMB 三元组 / 本地路径），
 * 所以同一部片重进能读回，换片不会串味。
 *
 * 【存储】SharedPreferences 的字符串浮点（避免 float 误差累积），空键不落盘。
 */
internal class SubtitleDelayMemory(context: Context) {

    private val sp = context.applicationContext
        .getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** 读该片源记住的**主字幕**延迟（秒）；没记过或键为空 ⇒ 0。 */
    fun load(sourceKey: String?): Double = load(MAIN_PREFIX, sourceKey)

    /** 读该片源记住的**次字幕**延迟（秒）；没记过 ⇒ 0（SUB-M5，与主延迟同口径）。 */
    fun loadSecondary(sourceKey: String?): Double = load(SECONDARY_PREFIX, sourceKey)

    /** 记住主字幕延迟。夹到 ±30 s（超过这个量级通常是取错了字幕）。 */
    fun save(sourceKey: String?, seconds: Double) = save(MAIN_PREFIX, sourceKey, seconds)

    /** 记住次字幕延迟（同一片源键、不同前缀，主次不互相覆盖）。 */
    fun saveSecondary(sourceKey: String?, seconds: Double) = save(SECONDARY_PREFIX, sourceKey, seconds)

    private fun load(prefix: String, sourceKey: String?): Double {
        val key = sourceKey?.takeIf { it.isNotBlank() } ?: return 0.0
        return sp.getString(prefix + key, null)?.toDoubleOrNull()
            ?.let { SubtitleStyle.clampDelay(it) } ?: 0.0
    }

    private fun save(prefix: String, sourceKey: String?, seconds: Double) {
        val key = sourceKey?.takeIf { it.isNotBlank() } ?: return
        val value = SubtitleStyle.clampDelay(seconds)
        if (value == 0.0) sp.edit().remove(prefix + key).apply()
        else sp.edit().putString(prefix + key, String.format(java.util.Locale.US, "%.2f", value)).apply()
    }

    private companion object {
        const val PREFS = "subtitle_delay"
        const val MAIN_PREFIX = "d:"
        const val SECONDARY_PREFIX = "s:"
    }
}
