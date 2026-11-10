package com.kivo.cinema.playback

import android.content.Context
import android.os.ParcelFileDescriptor
import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import java.util.concurrent.CountDownLatch
import java.util.concurrent.FutureTask
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/** Real proxy callbacks exercise the shared miss lock, cache bypass and cancellation. */
internal object RangeConcurrencyRegression {
    private const val length = 6 * 1024 * 1024

    fun run(context: Context) {
        sameHole(context)
        cachedWhileBlocked(context)
        cancelBlockedReaders(context)
    }

    private fun sameHole(context: Context) {
        val entered = CountDownLatch(1)
        val release = CountDownLatch(1)
        val readers = CountDownLatch(2)
        val requests = AtomicInteger()
        RangeProbeRegression.serve(length, requests = requests, beforeBody = { request ->
            if (request == 2) { entered.countDown(); await(release) }
        }) { url ->
            HttpRangeFile(context, url) { event ->
                if (event.startsWith("read_begin ")) readers.countDown()
            }.use { source ->
                source.prepare()
                source.openDescriptor().use { first ->
                    source.openDescriptor().use { second ->
                        val a = async { read(first, 0) }
                        val b = async { read(second, 0) }
                        try {
                            await(entered); await(readers)
                        } finally { release.countDown() }
                        a.get(2, TimeUnit.SECONDS); b.get(2, TimeUnit.SECONDS)
                        check(requests.get() == 2) { "concurrent same hole downloaded twice" }
                    }
                }
            }
        }
    }

    private fun cachedWhileBlocked(context: Context) {
        val entered = CountDownLatch(1)
        val release = CountDownLatch(1)
        RangeProbeRegression.serve(length, beforeBody = { request ->
            if (request == 3) { entered.countDown(); await(release) }
        }) { url ->
            HttpRangeFile(context, url).use { source ->
                source.prepare()
                source.openDescriptor().use { first ->
                    source.openDescriptor().use { second ->
                        read(first, 0)
                        val blocked = async { read(second, 3 * 1024 * 1024) }
                        try {
                            await(entered)
                            async { read(first, 0) }.get(500, TimeUnit.MILLISECONDS)
                        } finally { release.countDown() }
                        blocked.get(2, TimeUnit.SECONDS)
                    }
                }
            }
        }
    }

    private fun cancelBlockedReaders(context: Context) {
        val entered = CountDownLatch(1)
        val release = CountDownLatch(1)
        val readers = CountDownLatch(2)
        RangeProbeRegression.serve(length, beforeBody = { request ->
            if (request == 2) { entered.countDown(); await(release) }
        }) { url ->
            HttpRangeFile(context, url) { event ->
                if (event.startsWith("read_begin ")) readers.countDown()
            }.use { source ->
                source.prepare()
                source.openDescriptor().use { first ->
                    source.openDescriptor().use { second ->
                        val a = async { runCatching { read(first, 0) }.isFailure }
                        val b = async { runCatching { read(second, 65536) }.isFailure }
                        try {
                            await(entered); await(readers)
                            async { source.cancel() }.get(500, TimeUnit.MILLISECONDS)
                            check(a.get(1, TimeUnit.SECONDS) && b.get(1, TimeUnit.SECONDS)) {
                                "cancelled proxy reads must both fail"
                            }
                            check(source.failureReason == "播放已取消")
                        } finally { release.countDown() }
                    }
                }
            }
        }
    }

    private fun read(fd: ParcelFileDescriptor, offset: Int) {
        val bytes = ByteArray(65536)
        check(Os.pread(fd.fileDescriptor, bytes, 0, bytes.size, offset.toLong()) == bytes.size)
        check(bytes.contentEquals(ByteArray(bytes.size) { ((offset + it) % 251).toByte() }))
    }

    private fun await(latch: CountDownLatch) {
        check(latch.await(4, TimeUnit.SECONDS)) { "proxy concurrency barrier timed out" }
    }

    private fun <T> async(block: () -> T): FutureTask<T> = FutureTask(block).also { task ->
        Thread(task, "KivoRangeRegression").apply { isDaemon = true; start() }
    }
}
