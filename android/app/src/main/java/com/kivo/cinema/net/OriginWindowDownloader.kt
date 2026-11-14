package com.kivo.cinema.net

import java.util.concurrent.atomic.AtomicIntegerArray

/**
 * 273 v28/v32：**窗口的下载机制** —— 把一个窗口从源站拉下来，边下边公布。
 *
 * ## 为什么从 [OriginWindowCache] 里拆出来
 * "有哪些窗口、读侧怎么命中"是**缓存的策略**；
 * "怎么从源站把这一段字节拉回来"是**传输的机制** —— 两种变化原因。
 * 改传输（连接池、分块大小、分片数）不该动命中语义，反之亦然。
 *
 * ## 边下边给
 * 每读到一块就 [OriginWindow.publishSlice] 并叫醒读侧 ——
 * 片头窗口 4 MiB 在冷源上要十几秒，读侧每多等一毫秒都是白等。
 *
 * ## v32：分片并行（为什么可以并行）
 * 源站的吞吐是**每连接**封顶的，不是整站封顶 —— 真机实测（K40，同一段字节）：
 * 单连接 **≈700 KB/s**，8 条连接并起来 **≈2,483 KB/s**。
 * ⇒ 一段窗口拆成 K 片，每片一条连接**同时**下，墙钟时间降到约 1/K。
 *
 * 【为什么每片一个线程，而不是"一轮发多个"】一轮发多个 = 串行做完，
 * 前一个没回来时后一个还在排队 —— 那就等于没并发。
 *
 * 【各片写哪儿】各片写共享缓冲里**自己那一段**（不重叠），
 * 写完各自 [OriginWindow.publishSlice] ⇒ 读侧按连续前缀取。
 */
internal class OriginWindowDownloader(
    private val window: OriginWindow,
    /** 源被关掉 / 窗口被换掉 ⇒ 立刻停。 */
    private val stopped: () -> Boolean,
    /** 又到了一批字节 ⇒ 叫醒在等的读侧。 */
    private val onProgress: () -> Unit,
) {

    /** 下载结果：下了多少字节 + 有没有**完整跑完**（异常中断 ⇒ false）。 */
    class Outcome(val bytes: Int, val completed: Boolean)

    /**
     * 下载这个窗口。
     *
     * @return 下了多少字节 + 有没有跑完。
     */
    fun run(): Outcome {
        val client = window.client ?: return Outcome(0, false)
        val count = window.sliceCount
        val bytesPerSlice = AtomicIntegerArray(count)
        val donePerSlice = AtomicIntegerArray(count)

        if (count <= 1) {
            val got = downloadSlice(client, 0, bytesPerSlice, donePerSlice)
            return Outcome(got, donePerSlice.get(0) == 1)
        }

        val started = System.currentTimeMillis()
        // 【第 0 片先占连接】落点就在第 0 片里（窗口头部）⇒ 它最该先拿到
        // 池里那条**最早**空出来的连接（热连接 ≈300 ms，新建 ≈1000–2000 ms）。
        val first = openSlice(client, 0)

        val threads = ArrayList<Thread>(count - 1)
        for (i in 1 until count) {
            val thread = Thread({
                downloadSlice(client, i, bytesPerSlice, donePerSlice)
            }, "KivoWindowSlice-$i").apply {
                isDaemon = true
                start()
            }
            threads.add(thread)
        }
        // 第 0 片在当前线程上下（不白等一个线程起跑）。
        pump(first, 0, bytesPerSlice, donePerSlice)
        for (thread in threads) {
            runCatching { thread.join(SLICE_JOIN_TIMEOUT_MS) }
        }

        var bytes = 0
        var completed = true
        for (i in 0 until count) {
            bytes += bytesPerSlice.get(i)
            if (donePerSlice.get(i) != 1) completed = false
        }
        NetDiag.log(
            "[${window.tag}] window parts=$count bytes=$bytes ok=$completed " +
                "ms=${System.currentTimeMillis() - started}",
        )
        return Outcome(bytes, completed)
    }

    /** 下一片（线程里跑的）：开连接 + 下完。 */
    private fun downloadSlice(
        client: HttpOriginClient,
        index: Int,
        bytesOut: AtomicIntegerArray,
        doneOut: AtomicIntegerArray,
    ): Int {
        val stream = openSlice(client, index) ?: return 0
        pump(stream, index, bytesOut, doneOut)
        return bytesOut.get(index)
    }

    /**
     * 开**一片**的连接。
     *
     * @return 流；null = 这一片没开成（源站拒绝 / 回显的起点不对）。
     */
    private fun openSlice(client: HttpOriginClient, index: Int): HttpOriginClient.OriginStream? {
        val sliceStart = window.sliceStart(index)
        val sliceEnd = window.sliceEnd(index)
        if (sliceEnd <= sliceStart) return null
        return try {
            val stream = client.openRange(sliceStart, sliceEnd - 1)
            // 【核对源站回显】回显的起点必须与请求一致 ——
            // 否则这段字节不知道该放进缓冲的哪一段 ⇒ 这一片作废（不喂错字节）。
            if (stream.start != sliceStart) {
                NetDiag.log(
                    "[${window.tag}] slice#$index 起点回显不符 req=$sliceStart " +
                        "got=${stream.start} ⇒ 这一片作废",
                )
                stream.abort()
                null
            } else {
                stream
            }
        } catch (e: Exception) {
            NetDiag.log(
                "[${window.tag}] slice#$index fail: ${e.javaClass.simpleName}: ${e.message}",
            )
            null
        }
    }

    /** 把**一片**读完，边下边公布。 */
    private fun pump(
        stream: HttpOriginClient.OriginStream?,
        index: Int,
        bytesOut: AtomicIntegerArray,
        doneOut: AtomicIntegerArray,
    ) {
        if (stream == null) {
            bytesOut.set(index, 0)
            doneOut.set(index, 0)
            return
        }
        val sliceStart = window.sliceStart(index)
        val sliceEnd = window.sliceEnd(index)
        var got = 0
        var done = false
        try {
            val buf = window.buffer()
            val off = (sliceStart - window.actualStart).toInt()
            val want = (sliceEnd - sliceStart).toInt()
            while (got < want) {
                if (stopped()) break
                val n = stream.readInto(buf, off + got, want - got)
                if (n <= 0) break
                got += n
                window.publishSlice(index, got)
                onProgress()
            }
            done = true
        } catch (e: Exception) {
            NetDiag.log(
                "[${window.tag}] slice#$index fail: ${e.javaClass.simpleName}: ${e.message}",
            )
        } finally {
            // 【读完就 finish，不能 abort】读满了 ⇒ 响应体读完 ⇒
            // 连接回 keep-alive 池（下次复用，省一次 ~800 ms 握手）。
            // abort 是"中途放弃"，会把连接作废。
            if (stream.exhausted) stream.finish() else stream.abort()
        }
        bytesOut.set(index, got)
        doneOut.set(index, if (done) 1 else 0)
    }

    companion object {
        /**
         * 等一片收尾的上限（毫秒）。
         *
         * 【为什么要上限】窗口被换掉时旧片的线程可能还挂着；
         * 等它自己收尾就行，但不能无限等（源站半死不活时会把线程拖住）。
         */
        private const val SLICE_JOIN_TIMEOUT_MS = 30_000L
    }
}
