package com.kivo.cinema.subtitle.online

import android.content.Context
import com.kivo.cinema.tmdb.TmdbClient
import java.util.Locale

/** 轻量 GET（TmdbClient 只有 openConnection，没有通用 get；供两源复用）。 */
internal fun httpGet(client: TmdbClient, url: String): String? = runCatching {
    val conn = client.openConnection(url)
    try {
        if (conn.responseCode !in 200..299) return@runCatching null
        conn.inputStream.readBytes().decodeToString()
    } finally {
        conn.disconnect()
    }
}.getOrNull()

/** 在线字幕数据源（第一批两源，design.md §八 决策）。 */
enum class SubSource { OpenSubtitles, SubHd }

/** 一条真实在线字幕结果（OpenSubtitles attributes / SubHD 详情页字段，不编造）。 */
data class OnlineSubtitle(
    val source: SubSource,
    val id: String,
    val title: String,
    val languageCode: String,
    val languageLabel: String,
    val format: String,
    val downloads: Int,
    val ratingUp: Int,
    val ratingDown: Int,
    val uploader: String,
    val releaseName: String,
    val pageUrl: String,
    val fileId: Long = 0,
    val trusted: Boolean = false,
)

/** 语言归一：OpenSubtitles 语言码 / SubHD 文案 → 统一标签。 */
fun languageLabelOf(raw: String): String = when (val code = raw.lowercase().trim()) {
    "zh-cn", "zh-hans", "zh", "chs", "简体", "简体中文" -> "简体"
    "zh-tw", "zh-hant", "cht", "繁体", "繁體", "繁体中文" -> "繁体"
    "en", "eng", "英语", "英文" -> "英语"
    "双语", "中英", "简英", "繁英", "bilingual" -> "双语"
    else -> if (code.isBlank()) "其他" else raw
}

/**
 * 语言排序（用户决策 §八-3：按系统语言，不一刀切中文优先）。
 * 中文系统：简体 → 双语 → 繁体 → 英语 → 其他；
 * 非中文系统：英语 → 双语 → 中文（简/繁）→ 其他。
 */
fun languageRank(label: String, systemLocale: Locale): Int {
    val zh = systemLocale.language.equals("zh", ignoreCase = true)
    return if (zh) {
        when (label) {
            "简体" -> 0; "双语" -> 1; "繁体" -> 2; "英语" -> 3; else -> 4
        }
    } else {
        when (label) {
            "英语" -> 0; "双语" -> 1; "简体", "繁体" -> 2; else -> 3
        }
    }
}

/** 合并排序：语言偏好 → 下载量/评分 → 可信源。 */
fun sortOnlineSubtitles(items: List<OnlineSubtitle>, systemLocale: Locale): List<OnlineSubtitle> =
    items.sortedWith(
        compareBy(
            { languageRank(it.languageLabel, systemLocale) },
            { -it.downloads },
            { -it.ratingUp },
            { !it.trusted },
        ),
    )

/** 设置存储（SharedPreferences「online_subtitles」）：key/账号/自动选中。 */
class OnlineSubtitlePrefs(context: Context) {
    private val prefs = context.getSharedPreferences("online_subtitles", Context.MODE_PRIVATE)

    var apiKey: String
        get() = prefs.getString("api_key", "") ?: ""
        set(v) = prefs.edit().putString("api_key", v).apply()

    var username: String
        get() = prefs.getString("username", "") ?: ""
        set(v) = prefs.edit().putString("username", v).apply()

    var password: String
        get() = prefs.getString("password", "") ?: ""
        set(v) = prefs.edit().putString("password", v).apply()

    var bearerToken: String
        get() = prefs.getString("bearer", "") ?: ""
        set(v) = prefs.edit().putString("bearer", v).apply()

    var autoSelect: Boolean
        get() = prefs.getBoolean("auto_select", true)
        set(v) = prefs.edit().putBoolean("auto_select", v).apply()
}