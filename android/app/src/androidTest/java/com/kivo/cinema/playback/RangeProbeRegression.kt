package com.kivo.cinema.playback

import android.content.Context
import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import java.net.InetAddress
import java.net.ServerSocket
import java.util.concurrent.atomic.AtomicInteger

/** Reproduces the real CDN's one-byte size bug without credentials or external network. */
internal object RangeProbeRegression {
    fun run(context: Context): String {
        HttpByteCacheRegression.run()
        RangeConcurrencyRegression.run(context)
        val exhaustedRequests = AtomicInteger()
        serve(6 * 1024 * 1024, requests = exhaustedRequests) { url ->
            HttpRangeFile(context, url).use { source ->
                SharedRangeRegression.exhaustedIndex(source, exhaustedRequests, 6 * 1024 * 1024)
            }
        }
        val sharedRequests = AtomicInteger()
        serve(3 * 1024 * 1024, requests = sharedRequests) { url ->
            HttpRangeFile(context, url).use { source ->
                SharedRangeRegression.run(source, sharedRequests, 3 * 1024 * 1024)
            }
        }
        val continuousRequests = AtomicInteger()
        serve(6 * 1024 * 1024, requests = continuousRequests) { url ->
            HttpRangeFile(context, url).use { source ->
                SharedRangeRegression.continuous(source, continuousRequests, 6 * 1024 * 1024)
            }
        }
        for ((length, capBytes) in listOf(17 to 1024, 16384 to 16384, 16384 to 1024)) {
            serve(length, capBytes = capBytes) { url ->
                HttpRangeFile(context, url).use { source ->
                    source.prepare()
                    check(source.size == length.toLong())
                    source.openDescriptor().use { fd ->
                        if (length > 4096) {
                            val head = ByteArray(4096)
                            check(Os.pread(fd.fileDescriptor, head, 0, head.size, 0) == head.size)
                            check(head.contentEquals(ByteArray(head.size) { (it % 251).toByte() })) {
                                "fragmented HTTP body must not become zero-padded media bytes"
                            }
                        }
                        val bytes = ByteArray(8)
                        check(Os.pread(fd.fileDescriptor, bytes, 0, 8, (length - 8).toLong()) == 8)
                        check(bytes.contentEquals(ByteArray(8) { ((length - 8 + it) % 251).toByte() }))
                    }
                }
            }
        }
        serve(16384, invalidEnd = true) { url ->
            HttpRangeFile(context, url).use { source ->
                check(runCatching { source.prepare() }.isFailure)
                check(source.failureReason.isNotBlank())
            }
        }
        val requests = AtomicInteger()
        serve(3 * 1024 * 1024, requests = requests) { url ->
            HttpRangeFile(context, url).use { source ->
                source.prepare()
                source.openDescriptor().use { fd ->
                    for (offset in listOf(0, 768 * 1024 + 37, 96 * 1024 + 11,
                        900 * 1024 + 3, 64 * 1024 + 19)) {
                        val bytes = ByteArray(4096)
                        check(Os.pread(fd.fileDescriptor, bytes, 0, bytes.size, offset.toLong()) == bytes.size)
                        check(bytes.contentEquals(ByteArray(bytes.size) { ((offset + it) % 251).toByte() }))
                    }
                    check(requests.get() == 2) { "same-range random reads reopened HTTP: ${requests.get()} requests" }
                    for (offset in listOf(1024 * 1024 + 16384, 2 * 1024 * 1024 + 101,
                        2 * 1024 * 1024 - 777)) {
                        val bytes = ByteArray(8192)
                        check(Os.pread(fd.fileDescriptor, bytes, 0, bytes.size, offset.toLong()) == bytes.size)
                        check(bytes.contentEquals(ByteArray(bytes.size) { ((offset + it) % 251).toByte() }))
                        check(requests.get() == 2) { "continuous cross-window reads reopened HTTP: ${requests.get()}" }
                    }
                    // The head left this descriptor's 1MiB ring but remains in
                    // the shared sparse cache, with its original exact bytes.
                    val head = ByteArray(8192)
                    check(Os.pread(fd.fileDescriptor, head, 0, head.size, 0) == head.size)
                    check(head.contentEquals(ByteArray(head.size) { (it % 251).toByte() }))
                    check(requests.get() == 2) { "shared cached head reopened HTTP" }
                }
            }
        }
        return "PASS exhausted index preserves live media response (4 HTTP); concurrent same-hole single download; cached hit bypasses blocked I/O; cancel aborts network and lock-waiting readers; sparse cache holes/overlap/bounds/LRU/edges/concurrency; shared head/index and interleaved 4MiB continuous/index each 3 HTTP requests; probe/fragmented/capped/short-file exact bytes; oversized response rejected; continuous cross-window/wrapped/backward reads reuse one response (2 including probe)"
    }

    internal fun serve(length: Int, invalidEnd: Boolean = false, capBytes: Int = length,
                      requests: AtomicInteger = AtomicInteger(), beforeBody: (Int) -> Unit = {},
                      test: (String) -> Unit) {
        ServerSocket(0, 8, InetAddress.getByName("127.0.0.1")).use { server ->
            val clients = java.util.concurrent.CopyOnWriteArrayList<java.net.Socket>()
            val handlers = java.util.concurrent.CopyOnWriteArrayList<Thread>()
            val worker = Thread {
                while (!server.isClosed) {
                    val socket = try { server.accept() } catch (_: Exception) { break }
                    clients += socket
                    handlers += Thread {
                    socket.use { client ->
                        client.soTimeout = 3000
                        runCatching {
                            val reader = client.getInputStream().bufferedReader()
                            reader.readLine()
                            var requested = ""
                            while (true) {
                                val line = reader.readLine() ?: break
                                if (line.isEmpty()) break
                                if (line.startsWith("Range:", true)) requested = line.substringAfter(':').trim()
                            }
                            val bounds = requested.removePrefix("bytes=").split('-')
                            val request = requests.incrementAndGet()
                            val start = bounds[0].toInt()
                            val end = if (invalidEnd) 8191 else minOf(bounds[1].toInt(), length - 1, start + capBytes - 1)
                            // Actual observed CDN response for the old prepare() request.
                            val total = if (requested == "bytes=0-0") 1 else length
                            val body = ByteArray(end - start + 1) { ((start + it) % 251).toByte() }
                            client.getOutputStream().apply {
                                write(("HTTP/1.1 206 Partial Content\r\nContent-Range: bytes $start-$end/$total\r\n" +
                                    "Content-Length: ${body.size}\r\nConnection: close\r\n\r\n").toByteArray())
                                flush()
                                beforeBody(request)
                                // Force a short network read before the rest arrives.
                                write(body, 0, minOf(16, body.size))
                                flush()
                                Thread.sleep(60)
                                if (body.size > 16) write(body, 16, body.size - 16)
                                flush()
                            }
                        }
                    }
                    clients.remove(socket)
                    }.apply { isDaemon = true; start() }
                }
            }.apply { isDaemon = true; start() }
            try { test("http://127.0.0.1:${server.localPort}/probe") }
            finally {
                server.close(); worker.join(4000)
                clients.forEach { runCatching { it.close() } }
                handlers.forEach { it.join(4000); check(!it.isAlive) { "test HTTP handler leaked" } }
            }
        }
    }
}
