package com.kivo.cinema.net

import java.util.concurrent.atomic.AtomicIntegerArray

/**
 * 273 v18/v32：**一个窗口** —— 一段显式范围 + 它自己的字节缓冲（可分片并行）。
 *
 * ## 它是什么
 * 窗口缓存（[OriginWindowCache]）里的一个条目。它只管**自己那段字节**：
 * 下到哪儿了、能从某个偏移读出多少。
 *
 * ## 为什么单独一个文件
 * "一个窗口的数据生命周期"与"几个窗口怎么编排"是**两种变化原因**：
 * 改缓冲/发布语义不该动编排（谁先谁后），反之亦然。
 *
 * ## 缓冲为什么共享、为什么不用拷贝
 * 预取线程往同一块 [buf] 里**逐片写**，读侧从**同一块**里拷。
 * 写者只写自己那一片，读者只读已公布的前缀 ⇒ 两侧不重叠。
 *
 * 【为什么不能等整块下完才给读侧】片头窗口 4 MiB，冷源上要十几秒；
 * 读侧每等一毫秒都是白等（内核就在等这段字节）。**边下边给**才是对的。
 *
 * ## v32：分片并行（为什么可以并行）
 * 源站的吞吐是**每连接**封顶的，不是整站封顶 —— 真机实测（K40，同一段字节）：
 * 单连接 **≈700 KB/s**，8 条连接并起来 **≈2,483 KB/s**。
 * ⇒ 一段窗口用 **K 条连接**同时下，墙钟时间降到约 1/K。
 *
 * ## 记账：为什么只公布"连续前缀"
 * 每片各记一个"已下字节数"；窗口对外只公布**连续前缀**
 * （第一片没下完，后面的片就算下完了也不算数）。
 *
 * 【为什么】读侧是**从头顺序**读的，落点就在窗口头部 ⇒ 前缀之外的字节它用不上；
 * 而"下了多少"会虚报可用量（中间缺一段 ⇒ 读侧照样读不下去）。
 */
internal class OriginWindow(
    /** 请求的起点（含）。 */
    val start: Long,
    /** 请求的终点（不含）。 */
    val endExclusive: Long,
    /**
     * 这一窗口专用的源站 client（独立标签 ⇒ 证据里分得清哪一跳是预取）。
     *
     * null = **已经下好**的窗口（见 [ready]）—— 它不必再去源站。
     */
    val client: HttpOriginClient?,
    /** 日志里的短标签（`head` / `tail` / `resume`）。 */
    val tag: String,
    /** 读侧愿意为它等多久（毫秒）；0 = 不等（落不了就回源站）。 */
    val waitMs: Long,
    /** 并行分片数（1 = 一条连接下完；>1 = 每片一条连接同时下）。 */
    val parts: Int = 1,
) {

    /** 窗口状态。 */
    enum class State { PENDING, READY, FAILED }

    /** 预取在飞 / 已就绪 / 失败。 */
    @Volatile var state: State = State.PENDING

    /**
     * 这一窗口被**换掉**了（同一标签来了新的窗口）。
     *
     * 【为什么要它】落点窗口每次拖动都换 ⇒ 旧窗口的下载是白费的
     * （源站的流量也是白花的）⇒ 叫停它，别让它把 4 MiB 下完。
     */
    @Volatile var cancelled = false

    /**
     * 缓冲的基准偏移（= [start]）。
     *
     * 【为什么固定成请求的起点】分片并行时各片的落点是**按请求起点算**的
     * （源站回显的起点必须与请求一致，否则这段字节不知道该放哪 ⇒ 那一片作废）。
     */
    val actualStart: Long = start

    /** 窗口总长（字节）。 */
    val size: Long = (endExclusive - start).coerceAtLeast(0L)

    /** 分片数（受最小分片大小约束 —— 太小的片被表头开销吃掉）。 */
    val sliceCount: Int = computeSliceCount()

    /** 分片边界（sliceCount + 1 个，递增；第 i 片 = [bounds[i], bounds[i+1])）。 */
    private val sliceBounds = LongArray(sliceCount + 1)

    /** 每片已下的字节数（各片各写各的 ⇒ 用原子数组发布给读侧）。 */
    private val sliceFilled = AtomicIntegerArray(sliceCount)

    /** 共享缓冲（预取线程写、读侧拷）。 */
    @Volatile private var buf: ByteArray? = null

    init {
        for (i in 0..sliceCount) {
            sliceBounds[i] = start + size * i / sliceCount
        }
    }

    /** 预取线程：把**已经下好**的缓冲挂上来（见 [ready]）。 */
    fun attach(buffer: ByteArray) {
        buf = buffer
    }

    /**
     * 预取线程：取这块窗口的缓冲（**懒分配** —— 多片并行时只分一次）。
     *
     * 【为什么懒分配】缓冲大小 = 窗口大小（几 MiB）⇒ 宁可等到真要下时才分，
     * 也不在"建了窗口但没下"的时候占着内存。
     */
    fun buffer(): ByteArray {
        buf?.let { return it }
        synchronized(this) {
            buf?.let { return it }
            val bytes = ByteArray(size.coerceAtMost(Int.MAX_VALUE.toLong()).toInt())
            buf = bytes
            return bytes
        }
    }

    /** 预取线程：公布"第 [index] 片已经下了 [filledInSlice] 字节"。 */
    fun publishSlice(index: Int, filledInSlice: Int) {
        if (index < 0 || index >= sliceCount) return
        val cap = sliceCap(index).toInt()
        sliceFilled.set(index, minOf(filledInSlice, cap))
    }

    /** 已落地的**连续前缀**字节数（读侧能连续读到的量）。 */
    val filled: Int get() = contiguousFilled()

    /** 第 [index] 片的起点（含）。 */
    fun sliceStart(index: Int): Long = sliceBounds[index]

    /** 第 [index] 片的终点（不含）。 */
    fun sliceEnd(index: Int): Long = sliceBounds[index + 1]

    /** 从 [offset] 起还有多少字节可读（0 = 还没有）。 */
    fun bytesAt(offset: Long): Int {
        val within = offset - actualStart
        if (within < 0L) return 0
        return (contiguousFilled() - within).coerceAtLeast(0L).toInt()
    }

    /**
     * 从 [offset] 起拷最多 [len] 个字节。
     *
     * @return 拷到的字节数；0 = 现在还没有（调用方按"等一等"或"回源站"处理）。
     */
    fun copyTo(offset: Long, dst: ByteArray, dstOff: Int, len: Int): Int {
        val source = buf ?: return 0
        val within = offset - actualStart
        if (within < 0L) return 0
        val avail = contiguousFilled() - within
        if (avail <= 0L) return 0
        val n = minOf(len.toLong(), avail, source.size - within).toInt()
        if (n <= 0) return 0
        System.arraycopy(source, within.toInt(), dst, dstOff, n)
        return n
    }

    /** 拷一份前 [len] 个字节（给解析方用：它要的是**整块内容**，不是"读一段"）。 */
    fun snapshot(len: Int): ByteArray? {
        val source = buf ?: return null
        val n = minOf(len, contiguousFilled(), source.size)
        if (n <= 0) return null
        return source.copyOf(n)
    }

    /** 第 [index] 片应有多少字节。 */
    private fun sliceCap(index: Int): Long = sliceBounds[index + 1] - sliceBounds[index]

    /**
     * 连续前缀：前面若干片都下满了，才轮到下一片的部分进度。
     *
     * 【为什么】读侧顺序读 ⇒ 中间缺一段就是读不下去 ⇒ 只认连续的部分。
     */
    private fun contiguousFilled(): Int {
        var sum = 0L
        for (i in 0 until sliceCount) {
            val cap = sliceCap(i)
            val got = sliceFilled.get(i)
            if (got >= cap) {
                sum += cap
                continue
            }
            sum += got
            break
        }
        val limit = buf?.size?.toLong() ?: size
        return sum.coerceIn(0L, limit).toInt()
    }

    /** 分片数：不能超过"每片至少 [MIN_SLICE_BYTES]"能切出的份数。 */
    private fun computeSliceCount(): Int {
        if (parts <= 1 || size <= 0L) return 1
        val byMin = (size / MIN_SLICE_BYTES).toInt().coerceAtLeast(1)
        return parts.coerceAtMost(byMin).coerceAtLeast(1)
    }

    companion object {
        /**
         * 一片至少这么多字节。
         *
         * 【为什么是 256 KiB】真机实测：8 片 × 512 KiB 比 4 片 × 1 MiB **更慢**
         * （表头往返摊不薄）⇒ 片不能切太碎。
         */
        private const val MIN_SLICE_BYTES = 256L * 1024L

        /**
         * **已经下好**的字节 ⇒ 登记成一个 READY 窗口（不必再下一次）。
         *
         * 【为什么要有】cues 表我们自己已经下过一遍（解析索引要用），
         * 内核 seek 时**还会再读同一段** —— 不复用就是同一段字节下两遍，
         * 而内核那一次在源站要新建连接（真机 v27-a：表头 **1,424 ms**）。
         *
         * 【为什么这样最省】复用已有的字节 ⇒ 零额外下载、零额外往返。
         */
        fun ready(start: Long, bytes: ByteArray, tag: String): OriginWindow =
            OriginWindow(start, start + bytes.size, null, tag, 0L).also {
                it.attach(bytes)
                it.publishSlice(0, bytes.size)
                it.state = State.READY
            }
    }
}
