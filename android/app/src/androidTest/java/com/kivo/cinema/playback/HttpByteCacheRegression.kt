package com.kivo.cinema.playback

import com.kivo.cinema.net.HttpByteCache
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicReference

/** Tests the production cache's sparse validity, eviction and shared-reader contract. */
internal object HttpByteCacheRegression {
    fun run() {
        val page = HttpByteCache.PAGE_BYTES
        val cache = HttpByteCache(20L * page, 3, page.toLong(), page.toLong())
        val output = ByteArray(32) { 99 }
        cache.put(4, byteArrayOf(1, 2, 3), 0, 3)
        check(cache.copyTo(0, output, 0, 7) == 0 && output.all { it == 99.toByte() })
        check(cache.copyTo(4, output, 0, 6) == 3 && output.take(3) == listOf<Byte>(1, 2, 3))
        cache.put(6, byteArrayOf(8, 9), 0, 2)
        check(cache.copyTo(4, output, 0, 4) == 4 && output.take(4) == listOf<Byte>(1, 2, 8, 9))
        cache.put(19L * page, byteArrayOf(7), 0, 1)
        cache.put(2L * page, byteArrayOf(2), 0, 1)
        cache.put(3L * page, byteArrayOf(3), 0, 1)
        check(cache.pageCount() == 3 && cache.copyTo(2L * page, output, 0, 1) == 0)
        check(cache.copyTo(4, output, 0, 4) == 4 && cache.copyTo(19L * page, output, 0, 1) == 1)

        val lru = HttpByteCache(20L * page, 2, 0, 0)
        for (index in 1..2) lru.put(index.toLong() * page, byteArrayOf(index.toByte()), 0, 1)
        check(lru.copyTo(page.toLong(), output, 0, 1) == 1)
        lru.put(3L * page, byteArrayOf(3), 0, 1)
        check(lru.copyTo(2L * page, output, 0, 1) == 0)
        check(lru.copyTo(page.toLong(), output, 0, 1) == 1)
        val cross = HttpByteCache(page.toLong() + 5)
        val input = ByteArray(12) { (it + 1).toByte() }
        cross.put(page.toLong() - 7, input, 0, input.size)
        check(cross.copyTo(page.toLong() - 7, output, 2, 20) == 12)
        check(output.copyOfRange(2, 14).contentEquals(input))
        check(cross.copyTo(page.toLong() + 5, output, 0, 1) == 0)
        check(runCatching { cross.put(page.toLong(), input, 0, 6) }.isFailure)
        cross.clear()
        check(cross.pageCount() == 0 && cross.copyTo(page.toLong(), output, 0, 5) == 0)

        val concurrent = HttpByteCache(8L * page, 8, 0, 0)
        val ready = CountDownLatch(4)
        val failure = AtomicReference<Throwable?>()
        val threads = (0..3).map { worker ->
            Thread {
                try {
                    val bytes = ByteArray(page + 13) { (worker * 31 + it % 251).toByte() }
                    val offset = worker * 2L * page
                    ready.countDown()
                    ready.await()
                    repeat(12) {
                        concurrent.put(offset, bytes, 0, bytes.size)
                        val actual = ByteArray(bytes.size)
                        check(concurrent.copyTo(offset, actual, 0, actual.size) == actual.size)
                        check(actual.contentEquals(bytes))
                    }
                } catch (e: Throwable) { failure.compareAndSet(null, e) }
            }.apply { start() }
        }
        threads.forEach { it.join(5000); check(!it.isAlive) }
        failure.get()?.let { throw it }
        check(concurrent.pageCount() <= 8)
    }
}
