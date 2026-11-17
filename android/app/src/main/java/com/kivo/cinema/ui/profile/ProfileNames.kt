package com.kivo.cinema.ui.profile

/**
 * 观看 key（[com.kivo.cinema.stats.WatchStore] 里的 path）→ 上屏片名。
 *
 * 三条分支（PROF-01）：
 * 1. `file://` / `/storage` / `/sdcard` 本地路径 ⇒ 取文件名、去扩展名；
 * 2. `http(s)://` ⇒ 用对齐到的 [item] 片名（对齐由
 *    [com.kivo.cinema.library.ContinueWatching] 完成，与发现页同一套 key）；
 * 3. 其余（含解析失败）⇒ [UNKNOWN]。
 *
 * **任何分支都不允许把 URL/query 片段上屏**：形如 `://`、`?`、`&`、`=` 的候选名一律判为不可用。
 */
object ProfileNames {

    /** 解析不出来时的兜底片名，绝不回落到原始 key。 */
    const val UNKNOWN = "未知影片"

    private const val FILE_SCHEME = "file://"
    private const val HTTP_SCHEME = "http://"
    private const val HTTPS_SCHEME = "https://"
    private const val STORAGE_PREFIX = "/storage/"
    private const val SDCARD_PREFIX = "/sdcard/"
    private const val PATH_SEPARATOR = '/'
    private const val QUERY_MARKER = '?'
    private const val FRAGMENT_MARKER = '#'
    private const val EXTENSION_MARKER = '.'

    /** URL 特征字符：命中即视为不可上屏（宁可「未知影片」也不给用户看乱码）。 */
    private val URL_MARKERS = listOf("://", "?", "&", "=")

    /**
     * @param source 观看记录 key（可能是本地路径或网络 URL）
     * @param alignedTitle 已对齐的片源标题（对不上时为 null）
     */
    fun display(source: String, alignedTitle: String?): String = when {
        isHttp(source) -> usable(alignedTitle) ?: UNKNOWN
        isLocalPath(source) -> fileNameOf(source) ?: usable(alignedTitle) ?: UNKNOWN
        else -> usable(alignedTitle) ?: fileNameOf(source) ?: UNKNOWN
    }

    private fun isHttp(source: String): Boolean =
        source.startsWith(HTTP_SCHEME, ignoreCase = true) ||
            source.startsWith(HTTPS_SCHEME, ignoreCase = true)

    private fun isLocalPath(source: String): Boolean =
        source.startsWith(FILE_SCHEME) || source.startsWith(STORAGE_PREFIX) ||
            source.startsWith(SDCARD_PREFIX)

    /** 文件名去扩展名；先剥 scheme/query/fragment，避免 query 里的 `&` 混进片名。 */
    fun fileNameOf(source: String): String? {
        val noScheme = source.removePrefix(FILE_SCHEME)
        val noQuery = noScheme.substringBefore(QUERY_MARKER).substringBefore(FRAGMENT_MARKER)
        val name = noQuery.substringAfterLast(PATH_SEPARATOR)
        if (name.isBlank()) return null
        val stem = if (name.contains(EXTENSION_MARKER)) name.substringBeforeLast(EXTENSION_MARKER) else name
        return usable(stem)
    }

    private fun usable(candidate: String?): String? {
        val text = candidate?.trim().orEmpty()
        if (text.isEmpty()) return null
        return text.takeUnless { name -> URL_MARKERS.any { name.contains(it) } }
    }
}