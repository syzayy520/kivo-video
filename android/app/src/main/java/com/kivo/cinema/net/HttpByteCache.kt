package com.kivo.cinema.net

import java.util.BitSet

/** Session-local sparse bytes. A cache hit never moves an extractor's HTTP stream. */
internal class HttpByteCache(private val length: Long, private val maxPages: Int = 128,
                             private val headBytes: Long = 512 * 1024,
                             private val tailBytes: Long = 256 * 1024) {
    private class Page {
        val bytes = ByteArray(PAGE_BYTES)
        val valid = BitSet(PAGE_BYTES)
    }
    private val pages = LinkedHashMap<Long, Page>(16, 0.75f, true)

    init { require(length > 0 && maxPages > 0 && headBytes >= 0 && tailBytes >= 0) }

    /** Copies only the contiguous valid prefix; holes return zero, never padded bytes. */
    @Synchronized fun copyTo(offset: Long, data: ByteArray, start: Int, count: Int): Int {
        require(offset >= 0 && start >= 0 && count >= 0 && start <= data.size - count)
        val limit = minOf(count.toLong(), (length - offset).coerceAtLeast(0)).toInt()
        var copied = 0
        while (copied < limit) {
            val position = offset + copied
            val page = pages[position / PAGE_BYTES] ?: break
            val index = (position % PAGE_BYTES).toInt()
            if (!page.valid[index]) break
            val size = minOf(limit - copied, PAGE_BYTES - index, page.valid.nextClearBit(index) - index)
            page.bytes.copyInto(data, start + copied, index, index + size)
            copied += size
        }
        return copied
    }

    @Synchronized fun put(offset: Long, data: ByteArray, start: Int, count: Int) {
        require(offset >= 0 && offset <= length && count.toLong() <= length - offset &&
            start >= 0 && count >= 0 && start <= data.size - count)
        var copied = 0
        while (copied < count) {
            val position = offset + copied
            val key = position / PAGE_BYTES
            val page = pages[key] ?: Page().also {
                if (pages.size == maxPages) {
                    val victim = pages.keys.firstOrNull { candidate -> !isEdge(candidate) }
                        ?: pages.keys.first()
                    pages.remove(victim)
                }
                pages[key] = it
            }
            val index = (position % PAGE_BYTES).toInt()
            val size = minOf(count - copied, PAGE_BYTES - index)
            data.copyInto(page.bytes, index, start + copied, start + copied + size)
            page.valid.set(index, index + size)
            copied += size
        }
    }

    private fun isEdge(key: Long): Boolean = key * PAGE_BYTES < headBytes ||
        (key + 1) * PAGE_BYTES > length - tailBytes

    @Synchronized fun clear() = pages.clear()
    @Synchronized fun pageCount(): Int = pages.size

    companion object { const val PAGE_BYTES = 64 * 1024 }
}
