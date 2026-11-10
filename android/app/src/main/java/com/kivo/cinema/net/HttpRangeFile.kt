package com.kivo.cinema.net

import android.content.Context
import android.os.Handler
import android.os.HandlerThread
import android.os.ParcelFileDescriptor
import android.os.ProxyFileDescriptorCallback
import android.os.storage.StorageManager
import android.system.ErrnoException
import android.system.OsConstants
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

/** A seekable, bounded HTTP Range source for the existing native fd pipeline. */
class HttpRangeFile(context: Context, private val address: String,
                    private val trace: ((String) -> Unit)? = null) : RangeSource {
    private val storage = context.getSystemService(StorageManager::class.java)
    private val cancelled = AtomicBoolean(false)
    private val readers = CopyOnWriteArrayList<Reader>()
    private val connections = CopyOnWriteArrayList<HttpURLConnection>()
    private val readerIds = AtomicInteger()
    private var sharedBytes: HttpByteCache? = null
    private val ioLock = Any()
    // LRU response list, at most two 1MiB windows for this media session.
    // Independent absolute fd offsets do not require independent HTTP cursors.
    private val streams = mutableListOf<HttpRangeStream>()
    @Volatile override var size: Long = 0; private set
    @Volatile override var failureReason: String = ""; private set

    /** [RangeSource.open]：探测总长度，失败时写 [failureReason]。 */
    override fun open() = prepare()

    fun prepare() {
        try {
            require(URL(address).protocol in setOf("http", "https")) { "仅支持 HTTP/HTTPS 片源" }
            // Some media CDNs report bytes 0-0/1 for a one-byte probe, even
            // though ordinary bounded ranges report the full media length.
            val probeEnd = 4095L
            val connection = connect(0, probeEnd)
            try {
                val range = range(connection, 0)
                check(range.first <= probeEnd) { "服务器返回范围超出请求" }
                size = range.second
                check(size > 0) { "片源大小无效" }
                check(!cancelled.get()) { "播放已取消" }
                sharedBytes = HttpByteCache(size)
            } finally { disconnect(connection) }
        } catch (e: Exception) {
            if (failureReason.isBlank()) {
                failureReason = if (cancelled.get()) "播放已取消" else
                    e.message?.takeIf { it.isNotBlank() } ?: "网络片源准备失败（${e.javaClass.simpleName}）"
            }
            throw e
        }
    }

    /** Each extractor owns an independent fd and callback thread. */
    override fun openDescriptor(): ParcelFileDescriptor {
        check(size > 0 && !cancelled.get()) { "网络片源未就绪或已取消" }
        val reader = Reader()
        readers += reader
        try {
            return storage.openProxyFileDescriptor(ParcelFileDescriptor.MODE_READ_ONLY,
                reader, Handler(reader.thread.looper)) ?: error("系统无法创建网络文件描述符")
        } catch (e: Exception) {
            reader.onRelease()
            throw e
        }
    }

    /** Called off the UI thread; aborts HTTP before native joins a blocked reader. */
    override fun cancel() {
        cancelled.set(true)
        connections.forEach { runCatching { it.disconnect() } }
    }

    /** Only after all native users and descriptors have been released. */
    override fun close() {
        cancel()
        readers.toList().forEach { it.onRelease() }
        synchronized(ioLock) { streams.forEach { it.close() }; streams.clear() }
        sharedBytes?.clear()
    }

    private fun connect(start: Long, end: Long, readerId: Int = 0): HttpURLConnection {
        if (trace != null) emit("http_begin reader=$readerId offset=$start end=$end")
        check(!cancelled.get()) { "播放已取消" }
        val connection = URL(address).openConnection() as HttpURLConnection
        connection.connectTimeout = 8_000
        connection.readTimeout = 8_000
        connection.setRequestProperty("Range", "bytes=$start-$end")
        connection.setRequestProperty("Accept-Encoding", "identity")
        connections += connection
        if (cancelled.get()) { disconnect(connection); error("播放已取消") }
        return connection
    }

    // Return response end and total; never accept a 200 response masquerading as seekable data.
    private fun range(connection: HttpURLConnection, expectedStart: Long, readerId: Int = 0): Pair<Long, Long> {
        val code = connection.responseCode
        if (code != 206) throw IOException("服务器不支持范围读取（HTTP $code）")
        val match = Regex("bytes (\\d+)-(\\d+)/(\\d+)")
            .matchEntire(connection.getHeaderField("Content-Range") ?: "")
            ?: throw IOException("服务器缺少有效的 Content-Range")
        val start = match.groupValues[1].toLongOrNull() ?: -1
        val end = match.groupValues[2].toLongOrNull() ?: -1
        val total = match.groupValues[3].toLongOrNull() ?: -1
        if (start != expectedStart || end < start || total <= end ||
            (size > 0 && total != size)) throw IOException("服务器范围或片源大小发生变化")
        val encoding = connection.contentEncoding
        if (!encoding.isNullOrBlank() && !encoding.equals("identity", true))
            throw IOException("服务器未提供原始媒体字节")
        if (trace != null) emit("http_ready reader=$readerId offset=$start end=$end total=$total")
        return end to total
    }

    private fun emit(event: String) {
        trace?.let { observer -> runCatching { observer(event) } }
    }

    private fun disconnect(connection: HttpURLConnection) {
        connections.remove(connection)
        connection.disconnect()
    }

    /** Only under ioLock. cancel() can disconnect without waiting for this lock. */
    private fun response(position: Long, readerId: Int): HttpRangeStream {
        val existing = streams.filter { it.distance(position) != null }
            .minByOrNull { it.distance(position)!! }
        if (existing != null) {
            streams.remove(existing)
            streams += existing
            return existing
        }
        if (streams.size == 2) streams.removeAt(0).close()
        val connection = connect(position, size - 1, readerId)
        try {
            val end = range(connection, position, readerId).first
            if (end >= size) throw IOException("服务器返回范围超出请求")
            return HttpRangeStream(position, end, connection.inputStream,
                { disconnect(connection) },
                { at, bytes, start, count -> sharedBytes?.put(at, bytes, start, count) }
            ).also { streams += it }
        } catch (e: Exception) {
            disconnect(connection)
            throw e
        }
    }

    private inner class Reader : ProxyFileDescriptorCallback() {
        private val id = readerIds.incrementAndGet()
        val thread = HandlerThread("KivoHttpFd").apply { start() }
        private val released = AtomicBoolean(false)

        override fun onGetSize(): Long = size

        override fun onRead(offset: Long, size: Int, data: ByteArray): Int {
            val began = if (trace != null) System.nanoTime() else 0L
            if (trace != null) emit("read_begin reader=$id offset=$offset size=$size")
            try {
                if (cancelled.get() || released.get()) throw IOException("播放已取消")
                if (offset < 0 || size < 0) throw IOException("读取位置无效")
                if (offset >= this@HttpRangeFile.size) return 0
                if (size == 0) return 0
                val count = minOf(size.toLong(), data.size.toLong(), this@HttpRangeFile.size - offset).toInt()
                // Socket reads may stop at a packet boundary. A short proxy-file
                // read can be interpreted as EOF by the extractor/FUSE path.
                // Fill the requested file span, stopping only at the known range end.
                var filled = 0
                var networkBytes = 0L
                while (filled < count) {
                    if (cancelled.get() || released.get()) throw IOException("播放已取消")
                    val position = offset + filled
                    // Memory hits never wait behind another descriptor's network read.
                    val hit = sharedBytes?.copyTo(position, data, filled, count - filled) ?: 0
                    if (hit > 0) {
                        filled += hit
                        continue
                    }
                    synchronized(ioLock) {
                        if (cancelled.get() || released.get()) throw IOException("播放已取消")
                        // Another reader may have supplied this hole while we waited.
                        val cached = sharedBytes?.copyTo(position, data, filled, count - filled) ?: 0
                        if (cached > 0) filled += cached else {
                            val stream = response(position, id)
                            val read = stream.read(position, data, filled, count - filled) {
                                if (cancelled.get() || released.get()) throw IOException("播放已取消")
                            }
                            filled += read.copied
                            networkBytes += read.networkBytes
                            if (stream.exhausted) {
                                streams.remove(stream)
                                stream.close()
                            }
                        }
                    }
                }
                if (trace != null) emit("read_end reader=$id offset=$offset bytes=$filled network_bytes=$networkBytes elapsed_ms=${(System.nanoTime() - began) / 1_000_000}")
                return filled
            } catch (e: Exception) {
                if (trace != null) emit("read_error reader=$id offset=$offset type=${e.javaClass.simpleName} elapsed_ms=${(System.nanoTime() - began) / 1_000_000}")
                failureReason = if (cancelled.get()) "播放已取消" else when (e) {
                    is IOException -> if (e.message?.startsWith("服务器") == true ||
                        e.message == "网络片源提前结束") e.message!! else "网络读取失败（${e.javaClass.simpleName}）"
                    else -> "网络读取失败（${e.javaClass.simpleName}）"
                }
                throw ErrnoException("network read", OsConstants.EIO)
            }
        }

        override fun onRelease() {
            if (!released.compareAndSet(false, true)) return
            readers.remove(this)
            thread.quitSafely()
        }
    }
}
