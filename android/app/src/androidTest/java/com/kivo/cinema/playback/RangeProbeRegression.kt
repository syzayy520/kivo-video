package com.kivo.cinema.playback

import android.content.Context
import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import java.net.InetAddress
import java.net.ServerSocket

/** Reproduces the real CDN's one-byte size bug without credentials or external network. */
internal object RangeProbeRegression {
    fun run(context: Context): String {
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
        return "PASS bounded probe avoids CDN one-byte size bug; fragmented/capped responses and short files preserve exact bytes; oversized response rejected"
    }

    private fun serve(length: Int, invalidEnd: Boolean = false, capBytes: Int = length, test: (String) -> Unit) {
        ServerSocket(0, 8, InetAddress.getByName("127.0.0.1")).use { server ->
            val worker = Thread {
                while (!server.isClosed) {
                    val socket = try { server.accept() } catch (_: Exception) { break }
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
                            val start = bounds[0].toInt()
                            val end = if (invalidEnd) 8191 else minOf(bounds[1].toInt(), length - 1, start + capBytes - 1)
                            // Actual observed CDN response for the old prepare() request.
                            val total = if (requested == "bytes=0-0") 1 else length
                            val body = ByteArray(end - start + 1) { ((start + it) % 251).toByte() }
                            client.getOutputStream().apply {
                                write(("HTTP/1.1 206 Partial Content\r\nContent-Range: bytes $start-$end/$total\r\n" +
                                    "Content-Length: ${body.size}\r\nConnection: close\r\n\r\n").toByteArray())
                                // Force a short network read before the rest arrives.
                                write(body, 0, minOf(16, body.size))
                                flush()
                                Thread.sleep(60)
                                if (body.size > 16) write(body, 16, body.size - 16)
                                flush()
                            }
                        }
                    }
                }
            }.apply { isDaemon = true; start() }
            try { test("http://127.0.0.1:${server.localPort}/probe") }
            finally { server.close(); worker.join(4000) }
        }
    }
}
