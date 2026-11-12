package com.kivo.cinema.thumbnail

import android.media.MediaDataSource
import android.media.MediaExtractor
import com.kivo.cinema.net.ByteSource

/**
 * 缩略图读取器的**输入源**（v3.1）：本地路径 / 网络字节源，二选一。
 *
 * 【为什么要有网络源】v3 之前 [ThumbnailReader] 只吃本地路径，
 * SMB/mpv 链路没有缩略图 ⇒ 拖动进度条的预览浮窗永远是空框。
 * 这里把 [ByteSource]（SMB 读层）适配成 MediaExtractor 认识的 [MediaDataSource]，
 * 让网络片源也能独立 seek 解码缩略图（与主播放链路零干扰，215 设计 D1 的延伸）。
 */
internal sealed class ThumbInput {
    abstract fun applyTo(extractor: MediaExtractor)

    class Path(private val path: String) : ThumbInput() {
        override fun applyTo(extractor: MediaExtractor) = extractor.setDataSource(path)
    }

    class Source(private val dataSource: MediaDataSource) : ThumbInput() {
        override fun applyTo(extractor: MediaExtractor) = extractor.setDataSource(dataSource)
    }
}

/**
 * [ByteSource] → [MediaDataSource]。
 *
 * 契约要点：EOF 必须返回 **-1**（不是 0）；size=0 的读直接返回 0。
 * 读操作落在调用方线程（MediaExtractor 自己的线程），[ByteSource] 自带锁，
 * 但**必须给缩略图一条独立的源**——与主读共用会互相顶掉预读窗口（252 实证）。
 */
internal class ByteSourceMediaDataSource(private val source: ByteSource) : MediaDataSource() {

    override fun readAt(position: Long, buffer: ByteArray, offset: Int, size: Int): Int {
        if (size == 0) return 0
        if (position >= source.size()) return -1
        return source.readAt(position, buffer, offset, size)
    }

    override fun getSize(): Long = source.size()

    override fun close() = source.close()
}