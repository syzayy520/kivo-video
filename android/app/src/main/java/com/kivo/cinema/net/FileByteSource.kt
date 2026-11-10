package com.kivo.cinema.net

import java.io.RandomAccessFile

/** 本地文件的 [ByteSource] 实现（230 设计稿 D4）。 */
class FileByteSource(path: String) : ByteSource {

    private val raf = RandomAccessFile(path, "r")

    override fun size(): Long = try { raf.length() } catch (e: Exception) { -1 }

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int =
        try {
            synchronized(raf) {
                raf.seek(offset)
                raf.read(buf, off, len)
            }
        } catch (e: Exception) { -1 }

    override fun close() {
        try { raf.close() } catch (e: Exception) { }
    }
}
