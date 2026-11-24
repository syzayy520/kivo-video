package com.kivo.cinema.subtitle.online

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.coroutineScope
import java.util.Locale

/**
 * 两源并行查询与合并（design.md §三）：OpenSubtitles + SubHD 同时发，
 * 单源失败不拖垮另一源，失败原因并入 [notice]（不静默降级）。
 */
class OnlineSubtitleRepository(
    private val openSubtitles: OpenSubtitlesClient,
    private val subHd: SubHdClient,
) {
    data class Result(
        val items: List<OnlineSubtitle>,
        val notice: String?,
    )

    suspend fun search(query: String, year: Int? = null, locale: Locale = Locale.getDefault()): Result = coroutineScope {
        val errors = mutableListOf<String>()
        val os = async(Dispatchers.IO) {
            runCatching { openSubtitles.search(query, languageQuery(locale)) }
                .getOrElse {
                    errors += "OpenSubtitles：${it.message ?: "搜索失败"}"
                    emptyList()
                }
        }
        val hd = async(Dispatchers.IO) {
            runCatching { subHd.search(query, year) }
                .getOrElse {
                    errors += "SubHD：${it.message ?: "搜索失败"}"
                    emptyList()
                }
        }
        val items = sortOnlineSubtitles(os.await() + hd.await(), locale)
        Result(items, errors.joinToString("；").ifBlank { null })
    }

    private fun languageQuery(locale: Locale): String {
        val zh = locale.language.equals("zh", ignoreCase = true)
        return if (zh) "zh-cn,zh-tw,en" else "en,zh-cn,zh-tw"
    }
}