package com.kivo.cinema.net

import java.io.IOException
import java.io.InputStream

/** One bounded response window; HttpRangeFile serializes all access and owns cancellation. */
internal class HttpRangeStream(
    start: Long,
    private val end: Long,
    private val input: InputStream,
    private val closeResponse: () -> Unit,
    private val storeBytes: (Long, ByteArray, Int, Int) -> Unit,
) : AutoCloseable {
    private val cache = ByteArray(1024 * 1024)
    private var cachedStart = start
    private var cachedEnd = start

    // All actual bytes remain in the shared cache. A fully consumed response
    // must not evict another response that can still fetch media bytes.
    val exhausted: Boolean get() = cachedEnd == end + 1

    /** Prefer already buffered data, then the closest forward stream. Never download a huge gap. */
    fun distance(position: Long): Long? = when {
        position < cachedStart || position > end -> null
        position < cachedEnd -> 0
        position - cachedEnd <= cache.size -> position - cachedEnd + 1
        else -> null
    }

    data class Read(val copied: Int, val networkBytes: Long)

    fun read(position: Long, data: ByteArray, offset: Int, count: Int,
             checkActive: () -> Unit): Read {
        check(distance(position) != null)
        var copied = 0
        var networkBytes = 0L
        val limit = minOf(count.toLong(), end - position + 1).toInt()
        while (copied < limit) {
            checkActive()
            val at = position + copied
            if (at < cachedEnd) {
                val index = (at % cache.size).toInt()
                val want = minOf((limit - copied).toLong(), cachedEnd - at,
                    (cache.size - index).toLong()).toInt()
                cache.copyInto(data, offset + copied, index, index + want)
                copied += want
            } else {
                val needed = minOf(end + 1, at + minOf(limit - copied, cache.size))
                val index = (cachedEnd % cache.size).toInt()
                val want = minOf(needed - cachedEnd, (cache.size - index).toLong()).toInt()
                val read = input.read(cache, index, want)
                if (read <= 0) throw IOException("网络片源提前结束")
                storeBytes(cachedEnd, cache, index, read)
                networkBytes += read
                cachedEnd += read
                cachedStart = maxOf(cachedStart, cachedEnd - cache.size)
            }
        }
        return Read(copied, networkBytes)
    }

    override fun close() {
        runCatching { input.close() }
        closeResponse()
    }
}
