package com.kivo.cinema.net

/**
 * 273 v14：源站**响应头**的语义解析。
 *
 * ## 为什么单独一个文件
 * [HttpOriginClient] 管的是"连接怎么开、重定向怎么跟、失败怎么翻译"；
 * 这里管的是"这几个响应头字段是什么意思"。两种变化原因：
 * 源站换了（比如从 `Content-Range` 换到别的头）只改这里，
 * 连接策略（分块、复用、跳读）不动。
 *
 * ## 为什么必须看 `Content-Range` 而不是 `Content-Length`
 * 范围请求返回 206 时，`Content-Length` 是**这一段**的长度，
 * 片源总长在 `Content-Range: bytes START-END/TOTAL` 的斜杠后面。
 * 拿 `Content-Length` 当总长会让内核以为文件只有几兆 ⇒ 之后所有 seek 错位。
 */
internal object OriginHeadParser {

    /** 响应里带的 `Content-Range` 头（没有则 null）。 */
    fun contentRange(conn: java.net.HttpURLConnection): String? = conn.getHeaderField("Content-Range")

    /** 响应里带的 `Content-Length` 头（没有则 null）。 */
    fun contentLength(conn: java.net.HttpURLConnection): String? = conn.getHeaderField("Content-Length")

    /**
     * 片源总长。
     *
     * 优先取 `Content-Range` 的斜杠后总长（范围请求的标准位置）；
     * 退化时（206 但没有 Content-Range，或 200）用 `Content-Length`。
     */
    fun total(contentRange: String?, contentLength: String?, code: Int): Long? {
        val slash = contentRange?.substringAfterLast('/')?.trim()?.toLongOrNull()
        if (slash != null && slash > 0) return slash
        if (code == 200) {
            val length = contentLength?.trim()?.toLongOrNull()
            if (length != null && length > 0) return length
        }
        return null
    }

    /** 从 `Content-Range: bytes 123-456/789` 里取起点。 */
    fun rangeStart(header: String?): Long? =
        header?.trim()?.removePrefix("bytes ")?.substringBefore('/')?.substringBefore('-')?.trim()?.toLongOrNull()

    /** 从 `Content-Range: bytes 123-456/789` 里取终点（含）。 */
    fun rangeEnd(header: String?): Long? =
        header?.trim()?.removePrefix("bytes ")?.substringAfter('-')?.substringBefore('/')?.trim()?.toLongOrNull()
}
