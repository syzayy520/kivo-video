package com.kivo.cinema.net

/**
 * 273 v18：**EBML 原语** —— Matroska 容器里"一个元素"怎么读。
 *
 * ## 它是什么
 * EBML 的元素 = **ID + 长度 + 内容**。
 * · **ID**：1–4 字节，靠**前导零数**定长（标记位算进值）。
 * · **长度**：1–8 字节的 vint，靠前导零数定长（标记位**不算**进值）；
 *   全 1 表示"长度未知"（只能到本缓冲末尾）。
 *
 * ## 为什么单独一个文件
 * "EBML 这门字节格式"与"cues 的语义（时间 ↔ 字节）"是**两种变化原因**：
 * 改容器语义不该动字节解析，反之亦然。
 *
 * ## 失败语义
 * 任何一处对不上（越界、长度非法、不是合法 ID）⇒ 返回 -1 / 空列表。
 * 调用方按"没有索引"处理 —— **宁可慢，绝不把错字节喂给内核**。
 */
internal object MkvEbml {

    /** EBML 头（文件第 0 字节）。 */
    const val ID_EBML = 0x1A45DFA3

    /** Segment（容器主体）。 */
    const val ID_SEGMENT = 0x18538067

    /** SeekHead（索引表：给出 Cues 等元素在文件里的位置）。 */
    const val ID_SEEK_HEAD = 0x114D9B74
    const val ID_SEEK = 0x4DBB
    const val ID_SEEK_ID = 0x53AB
    const val ID_SEEK_POSITION = 0x53AC

    /** Info（片长、时间刻度）。 */
    const val ID_INFO = 0x1549A966

    /** TimecodeScale：一个刻度等于多少纳秒。 */
    const val ID_TIMECODE_SCALE = 0x2AD7B1

    /** Cues（"时间 → 簇"的索引表）。 */
    const val ID_CUES = 0x1C53BB6B
    const val ID_CUE_POINT = 0xBB
    const val ID_CUE_TIME = 0xB3
    const val ID_CUE_TRACK_POSITIONS = 0xB7
    const val ID_CUE_TRACK = 0xF7
    const val ID_CUE_CLUSTER_POSITION = 0xF1

    /** Tracks（轨道表：哪条是视频轨）。 */
    const val ID_TRACKS = 0x1654AE6B
    const val ID_TRACK_ENTRY = 0xAE
    const val ID_TRACK_NUMBER = 0xD7
    const val ID_TRACK_TYPE = 0x83

    /** TrackType = 1（视频轨）。 */
    const val TRACK_TYPE_VIDEO = 1L

    /** 长度读出来是"未知"（全 1）。 */
    const val UNKNOWN_LENGTH = -2L

    /**
     * 一个元素：ID 与内容区间。
     *
     * @param contentLength **声明**的内容长度（EBML 头里写的那个数）。
     *   读整数时**必须**用它 —— 用"被缓冲夹住后的长度"会把一个截断的数
     *   读成一个更小的合法值（那就是一个**错字节**）。
     * @param contentEnd 内容在缓冲里的真实结束（被缓冲末尾夹住）。
     */
    class Element(
        val id: Int,
        val contentStart: Int,
        val contentLength: Long,
        val contentEnd: Int,
    ) {
        /** 内容是否被缓冲末尾截断（截断 ⇒ 这个元素不算"读全了"）。 */
        val truncated: Boolean get() = contentStart + contentLength > contentEnd
    }

    /**
     * 标记位定长：按首字节的前导零数算这个 vint/ID 占几个字节。
     *
     * @return 字节数；-1 = 不是合法的定长（首字节全 0）。
     */
    private fun markerLen(first: Int, max: Int): Int {
        var len = 1
        var mask = 0x80
        while (len <= max && (first and mask) == 0) {
            mask = mask shr 1
            len++
        }
        return if (len > max) -1 else len
    }

    /** 元素 ID（1–4 字节）；-1 = 不是一个合法 ID。 */
    fun readId(buf: ByteArray, off: Int): Int {
        val first = buf.getOrNull(off)?.toInt() ?: return -1
        val len = markerLen(first, 4)
        if (len < 0 || off + len > buf.size) return -1
        var id = 0
        for (i in 0 until len) id = id * 256 + (buf[off + i].toInt() and 0xFF)
        return id
    }

    /**
     * 元素长度（vint）。
     *
     * @return 长度；-1 = 非法；[UNKNOWN_LENGTH] = "长度未知"（到缓冲末尾）。
     */
    fun readLength(buf: ByteArray, off: Int): Long {
        val first = buf.getOrNull(off)?.toInt() ?: return -1L
        val len = markerLen(first, 8)
        if (len < 0 || off + len > buf.size) return -1L
        val mask = 0x80 shr (len - 1)
        var value = (first and (mask - 1)).toLong()
        var allOnes = value == mask - 1L
        for (i in 1 until len) {
            val b = buf[off + i].toInt() and 0xFF
            value = value * 256 + b
            if (b != 0xFF) allOnes = false
        }
        return if (allOnes) UNKNOWN_LENGTH else value
    }

    /**
     * 读一个元素的**头**（不含内容）。
     *
     * 【内容区间会被缓冲末尾夹住】窗口里只装得下一半的元素 ⇒ 内容区间到末尾为止。
     * 调用方逐个读子元素时，**读全了的才算数**（见 [MkvCueIndex]）。
     */
    fun elementAt(buf: ByteArray, off: Int): Element? {
        if (off < 0 || off >= buf.size) return null
        val id = readId(buf, off)
        if (id < 0) return null
        val idLen = markerLen(buf[off].toInt(), 4)
        val lenPos = off + idLen
        val len = readLength(buf, lenPos)
        if (len < 0L && len != UNKNOWN_LENGTH) return null
        val lenLen = markerLen(buf[lenPos].toInt(), 8)
        val contentStart = lenPos + lenLen
        if (contentStart > buf.size) return null
        val declared = if (len == UNKNOWN_LENGTH) (buf.size - contentStart).toLong() else len
        val contentEnd = minOf(buf.size.toLong(), contentStart + declared).toInt()
        if (contentEnd < contentStart) return null
        return Element(id, contentStart, declared, contentEnd)
    }

    /** 在 [start, end) 里逐个读**同级**元素。 */
    fun children(buf: ByteArray, start: Int, end: Int): List<Element> {
        val out = ArrayList<Element>()
        var off = start
        while (off < end && off < buf.size) {
            val el = elementAt(buf, off) ?: break
            out.add(el)
            if (el.contentEnd <= off) break
            off = el.contentEnd
        }
        return out
    }

    /** 无符号整数（大端，1–8 字节）。 */
    fun readUint(buf: ByteArray, off: Int, len: Int): Long {
        if (len <= 0 || len > 8 || off < 0 || off + len > buf.size) return -1L
        var v = 0L
        for (i in 0 until len) v = v * 256 + (buf[off + i].toInt() and 0xFF)
        return v
    }
}
