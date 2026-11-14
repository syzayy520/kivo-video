package com.kivo.cinema.net

/**
 * 273 v5：预读环形缓冲 —— **供给侧与消费侧共享的那一份数据**。
 *
 * 【它只负责一件事】把"已经从源站下载、还没交给内核"的字节放在一个环里，
 * 消费侧 [copyOut] 取、供给侧 [copyIn] 写，两边通过同一把锁同步。
 *
 * 【为什么缓冲要独立成文件】连接策略（开哪一块、什么时候读完、慢读换连接）跟源站抖动
 * 的规律走，读语义（顺序读 / 跳读 / 关闭）跟内核的调用契约走 —— **两种变化
 * 原因**。合成一个文件，改连接策略就会踩到读语义，反之亦然。
 *
 * 【同步】一把 monitor + wait/notifyAll：
 *  · 消费侧等"有数据"（[awaitReadable]）；
 *  · 供给侧等"有空间"（[awaitRoom]）。
 * 共用一把锁 ⇒ 一次唤醒就能同时检查两个条件。
 */
class OriginPrefetchBuffer(
    val capacityBytes: Int,
    startOffset: Long,
) {
    private val monitor = Object()
    private val ring = ByteArray(capacityBytes)

    /** 下一个交给消费方的字节位置。 */
    private var readIdx = startOffset

    /** 供给侧下一个要写的字节位置。 */
    private var writeIdx = startOffset

    private var closed = false
    private var eof = false
    private var failure: Exception? = null
    private var total = -1L

    val readPosition: Long get() = synchronized(monitor) { readIdx }
    val writePosition: Long get() = synchronized(monitor) { writeIdx }
    val freeSpace: Long get() = synchronized(monitor) { capacityBytes - (writeIdx - readIdx) }

    /**
     * 环里已下好、还没交给内核的字节数（273 v29：让路闸看它决定让不让路）。
     */
    val bufferedBytes: Long get() = synchronized(monitor) { writeIdx - readIdx }
    val isClosed: Boolean get() = synchronized(monitor) { closed }
    val reachedEnd: Boolean get() = synchronized(monitor) { eof }
    val lastFailure: Exception? get() = synchronized(monitor) { failure }
    val knownTotal: Long get() = synchronized(monitor) { total }

    /** 等到环里有数据（或 eof / 失败 / 关闭）。true = 有数据可读。 */
    fun awaitReadable(timeoutMs: Long): Boolean = synchronized(monitor) {
        val deadline = System.currentTimeMillis() + timeoutMs
        while (writeIdx - readIdx <= 0 && !eof && failure == null && !closed) {
            val remain = deadline - System.currentTimeMillis()
            if (remain <= 0) return@synchronized false
            monitor.wait(remain)
        }
        writeIdx - readIdx > 0
    }

    /** 等到环里有空间（或关闭）。true = 有空间可写。 */
    fun awaitRoom(timeoutMs: Long): Boolean = synchronized(monitor) {
        val deadline = System.currentTimeMillis() + timeoutMs
        while (capacityBytes - (writeIdx - readIdx) <= 0 && !closed) {
            val remain = deadline - System.currentTimeMillis()
            if (remain <= 0) return@synchronized false
            monitor.wait(remain)
        }
        capacityBytes - (writeIdx - readIdx) > 0
    }

    /**
     * 消费侧取数据。
     *
     * @param offset 必须等于当前读位置（跳读由上层先做）。
     * @return 取到的字节数（0 = 位置不对或此刻没数据）。
     */
    fun copyOut(offset: Long, buf: ByteArray, off: Int, len: Int): Int = synchronized(monitor) {
        if (offset != readIdx || len <= 0) return@synchronized 0
        val avail = minOf(len.toLong(), writeIdx - readIdx).toInt()
        if (avail <= 0) return@synchronized 0
        val start = (readIdx % capacityBytes).toInt()
        val first = minOf(avail, capacityBytes - start)
        System.arraycopy(ring, start, buf, off, first)
        if (first < avail) System.arraycopy(ring, 0, buf, off + first, avail - first)
        readIdx += avail
        monitor.notifyAll()
        avail
    }

    /** 供给侧写数据：调用方必须先确认有空间（[awaitRoom]）。 */
    fun copyIn(src: ByteArray, off: Int, len: Int): Int = synchronized(monitor) {
        val room = (capacityBytes - (writeIdx - readIdx)).toInt()
        val got = minOf(room, len)
        if (got <= 0) return@synchronized 0
        val start = (writeIdx % capacityBytes).toInt()
        val first = minOf(got, capacityBytes - start)
        System.arraycopy(src, off, ring, start, first)
        if (first < got) System.arraycopy(src, off + first, ring, 0, got - first)
        writeIdx += got
        monitor.notifyAll()
        got
    }

    /** 跳读：清空环，把读写游标都挪到 [offset]。 */
    fun resetTo(offset: Long) = synchronized(monitor) {
        readIdx = offset
        writeIdx = offset
        eof = false
        monitor.notifyAll()
    }

    fun setTotal(value: Long) = synchronized(monitor) {
        if (value > 0) total = value
    }

    fun markEndOfFile() = synchronized(monitor) {
        eof = true
        monitor.notifyAll()
    }

    fun markFailure(error: Exception) = synchronized(monitor) {
        if (failure == null) failure = error
        monitor.notifyAll()
    }

    fun close() = synchronized(monitor) {
        closed = true
        monitor.notifyAll()
    }
}
