package com.kivo.cinema.smb

/**
 * 预读环形缓冲（250 D4）。纯数据结构：**不加锁、不开线程、不碰网络**。
 *
 * 从 [SmbReadAhead] 拆出来的理由不是行数，是可测性：代际校验、前移、边界判定
 * 这三件事以前只能靠真机播放来验。现在它们是纯函数，可以在 JVM 上把
 * "消费者跳到别处时，旧数据必须被丢弃"这条写成断言。
 *
 * **线程约定**：所有方法都必须在调用方的锁内使用，本类自身不做任何同步。
 * 唯一的跨线程交接是 [append] 的代际校验 —— 由调用方在同一把锁内比较 epoch。
 */
internal class SmbReadRing(private val capacity: Int) {

    private var data = ByteArray(0)
    private var baseOffset = 0L
    private var count = 0
    private var readPos = 0
    private var epoch = 0

    /** 缓冲是否已分配。首次 [reset] 时才分配，浏览/探测场景不付这笔内存。 */
    val ready: Boolean get() = data.isNotEmpty()

    /** 复位代际。消费者跳读时 +1，预读线程回写前必须核对。 */
    val epochValue: Int get() = epoch

    /** 缓冲中第一个字节对应的文件偏移。 */
    val base: Long get() = baseOffset

    /** 已灌入的字节数。 */
    val filled: Int get() = count

    /** 消费者已取走的字节数（相对 [base]）。 */
    val position: Int get() = readPos

    /** 下一次预读该从哪里写、还能写多少。 */
    val writable: Int get() = capacity - count
    val writeOffset: Long get() = baseOffset + count

    /** 把基点移到 [offset] 并清空。 */
    fun reset(offset: Long) {
        if (data.isEmpty()) data = ByteArray(capacity)
        baseOffset = offset
        count = 0
        readPos = 0
        ++epoch
    }

    /** 丢掉全部内容并换一代。连接换过、或访问变成乱序时用。 */
    fun invalidate() {
        count = 0
        readPos = 0
        ++epoch
    }

    /**
     * 请求是否**完整**落在未消费区间内。是则返回缓冲内起始索引，否则 -1。
     *
     * 部分命中不算：拼两次网络读更贵，而且拼出来的顺序更难保证。
     */
    fun indexOf(offset: Long, want: Int): Int {
        if (count <= readPos) return -1
        val start = baseOffset + readPos
        if (offset < start || offset + want > baseOffset + count) return -1
        return (offset - baseOffset).toInt()
    }

    fun copyFrom(index: Int, buf: ByteArray, off: Int, want: Int) {
        System.arraycopy(data, index, buf, off, want)
    }

    /** 标记"截至该位置已被消费"，让后续请求知道可用区间的起点。 */
    fun consume(index: Int, want: Int) {
        readPos = index + want
    }

    /**
     * 消费者取走一半以上就把剩余部分前移，避免缓冲被"取过的部分"占死。
     * 阈值取容量一半：更小会频繁搬运，更大则可用区间长期偏小。
     */
    fun compact() {
        if (readPos < capacity / 2) return
        val remain = count - readPos
        System.arraycopy(data, readPos, data, 0, remain)
        baseOffset += readPos
        count = remain
        readPos = 0
    }

    /**
     * 把 [length] 字节追加到末尾。**代际不符则整批丢弃** ——
     * 消费者可能在这次网络读期间跳读了，把旧位置的数据追加进新位置的缓冲
     * 会让画面从错的地方开始（花屏/错帧级）。返回是否写入。
     */
    fun append(epochAtRead: Int, source: ByteArray, length: Int): Boolean {
        if (epochAtRead != epoch || length > capacity - count) return false
        System.arraycopy(source, 0, data, count, length)
        count += length
        return true
    }
}
