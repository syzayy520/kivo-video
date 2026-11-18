package com.kivo.cinema.tmdb

/**
 * TMDB 请求语言：**跟随系统语言**，不再是写死的 `zh-CN`。
 *
 * ## 为什么只给中文区分地区
 * TMDB 的 `language` 接受 `xx` 或 `xx-YY` 两种形态。中文是特例——它按地区分成
 * `zh-CN`（简体）/ `zh-TW`（台湾正体）/ `zh-HK`（香港），给 `zh` 会拿到不稳定的结果。
 * 其它语言一律用两字母码（`en` / `ja` / `ko` / `fr`…）：TMDB 会给该语言的默认译文，
 * 比硬凑一个 `en-GB` 更稳（凑错地区时 TMDB 会返回空标题）。
 *
 * ## 兜底
 * 任何语言都可能遇到"该条目没有这个语言的翻译"（返回空 title）。调用方必须用
 * `original_title` / `original_name` 兜底，**不能把空标题显示成空白卡片**。
 */
object TmdbLocale {

    /** 真机验证用：`adb shell setprop debug.kivo.tmdbLang en-US`（不用改系统语言就能验其它语言）。 */
    private const val OVERRIDE_PROP = "debug.kivo.tmdbLang"

    /** 当前应使用的 TMDB 语言标签（如 `zh-CN` / `en` / `ja`）。 */
    fun tag(): String {
        sysProp(OVERRIDE_PROP)?.trim()?.takeIf { it.isNotEmpty() }?.let { return it }
        val loc = java.util.Locale.getDefault()
        val lang = loc.language.lowercase()
        if (lang == "zh") {
            return when (loc.country.uppercase()) {
                "TW" -> "zh-TW"
                "HK", "MO" -> "zh-HK"
                else -> "zh-CN"
            }
        }
        return lang.ifBlank { "en" }
    }

    /**
     * 读 Android 系统属性（`adb shell setprop` 写的值）。
     *
     * 【为什么不用 System.getProperty】那是 JVM 属性，与 Android 的 `setprop` 是两套东西，
     * 读 `debug.*` 属性必须走 `android.os.SystemProperties`（非 SDK API，反射访问）。
     * 失败一律返回 null —— 覆盖开关读不到就按系统语言走，不能因此崩。
     */
    private fun sysProp(key: String): String? = runCatching {
        val cls = Class.forName("android.os.SystemProperties")
        val get = cls.getMethod("get", String::class.java)
        get.invoke(null, key) as? String
    }.getOrNull()

    /** 图片语言优先级（logo/海报挑图用）：当前语言 → 英文 → 无文字图。 */
    fun imageLanguages(): String {
        val primary = tag().substringBefore('-')
        return "$primary,en,null"
    }
}
