package com.kivo.cinema.net

/**
 * 273 v26：**cues 索引的来源** —— 把片源的 MKV cues 表读成一张"时间 → 字节"的表。
 *
 * ## 它解决什么
 * 远跳（拖动进度条）之后内核要重新取数。真机实测（v24）：拖到 1.6 GB 处时，
 * 内核那一跳在源站**新建了一条连接**，表头 **1,274 ms** ⇒ 画面停 **0.597 s**。
 * 落点那一簇如果**事先在本地**，那一跳就不必新建连接 ⇒ 停顿消失。
 *
 * ## 为什么必须"两段取"
 * cues 表在**片尾**，而片尾窗口（64 KiB）未必盖得住它的开头 ——
 * 真机：cues 起点比片尾窗口的起点还早 **64,958 字节** ⇒ 盖不住 ⇒ 解析不出来。
 * ⇒ 先从**片头**的 SeekHead 拿到 cues 的精确位置，再按那个位置去取。
 *
 * ## 为什么不省这一次请求
 * 片头那一段本来就要下（内核起播第一跳就是它）⇒ 能从窗口缓存里拿就**不多下**；
 * 拿不到才自己下 64 KiB。cues 本体真机 ≈130 KB —— 一次小请求。
 *
 * ## 失败语义
 * 解析不出来 ⇒ 索引为 null ⇒ **不预取**（拖动照常走网络，功能不缺）。
 * 索引只覆盖**读全了**的那段 ⇒ 超出覆盖范围的时间一律不预取（绝不猜偏移）。
 */
internal class OriginCueIndexSource(
    /** 专用的源站 client（独立标签 ⇒ 证据里一眼分得清这一跳）。 */
    private val client: HttpOriginClient,
    /** 窗口缓存（片头窗口已下好时直接用它 ⇒ 一下不多下）。 */
    private val cache: OriginWindowCache,
) {

    /** 解析好的索引；null = 还没有 / 解析不出来。 */
    @Volatile private var parsed: MkvCueIndex? = null

    /** 后台任务已在飞（避免重复起线程）。 */
    @Volatile private var warming = false

    /** 索引（null = 不可用）。 */
    val index: MkvCueIndex? get() = parsed

    /**
     * 后台把索引读出来（幂等）。
     *
     * 【为什么放后台】它要两次源站往返（片头 + cues），
     * 而拖动随时可能发生 ⇒ 提前备好，拖动时**零等待**。
     */
    fun warmAsync() {
        if (parsed != null || warming) return
        warming = true
        Thread({
            try {
                warm()
            } finally {
                warming = false
            }
        }, "KivoCueIndex").apply {
            isDaemon = true
            start()
        }
    }

    /**
     * 同步读一遍索引。
     *
     * @return true = 索引已就绪。
     */
    fun warm(): Boolean {
        if (parsed != null) return true
        val started = System.currentTimeMillis()
        val (headStart, headBytes) = readHead() ?: return false
        val info = MkvCueIndex.parseHead(headBytes, headStart) ?: run {
            NetDiag.log("[cue] 片头里没有 SeekHead/Info ⇒ 放弃（不预取）")
            return false
        }
        val cues = readCues(info.cuesAbs) ?: return false
        val index = MkvCueIndex.parseCues(cues, info) ?: run {
            NetDiag.log("[cue] cues 解析不出来 ⇒ 放弃（不预取）")
            return false
        }
        parsed = index
        NetDiag.log(
            "[cue] cues 就绪 ${index.count} 条 cuesAbs=${info.cuesAbs} " +
                "ms=${System.currentTimeMillis() - started}",
        )
        return true
    }

    /**
     * 片头字节。
     *
     * 【优先用窗口缓存】详情页预热时片头窗口（4 MiB）已经下好 ⇒ 直接用它，
     * 一下不多下；没有窗口才自己取 64 KiB。
     */
    private fun readHead(): Pair<Long, ByteArray>? {
        val snap = cache.awaitWindow(
            OriginWindowCache.TAG_HEAD, HEAD_WAIT_MS, OriginWindowCache.PARSE_MIN_BYTES,
        )
        if (snap != null) {
            val head = MkvCueIndex.parseHead(snap.data, snap.start)
            if (head != null) {
                NetDiag.log("[cue] 片头取自窗口缓存 bytes=${snap.data.size}")
                return snap.start to snap.data
            }
        }
        val buf = fetch(0L, HEAD_PROBE_BYTES) ?: return null
        NetDiag.log("[cue] 片头自己取 bytes=${buf.size}")
        return 0L to buf
    }

    /**
     * cues 本体（缓冲从 Cues 元素的第一个字节开始）。
     *
     * 【两段取】先探 [CUES_PROBE_BYTES]：元素头里写着它自己的长度；
     * 探不够就按那个长度**精确**再取一次。
     */
    private fun readCues(cuesAbs: Long): ByteArray? {
        // 【先看片尾窗口在不在】预热路径上片尾窗口（256 KiB）盖得住 cues
        // ⇒ 一下不多下；直接播放路径上片尾窗口只有 64 KiB ⇒ 盖不住 ⇒ 走下面取。
        val snap = cache.awaitWindow(OriginWindowCache.TAG_TAIL, TAIL_WAIT_MS)
        if (snap != null) {
            val at = cuesAbs - snap.start
            if (at >= 0L && at < snap.data.size) {
                val el = MkvEbml.elementAt(snap.data, at.toInt())
                if (el != null && el.id == MkvEbml.ID_CUES && !el.truncated) {
                    NetDiag.log("[cue] cues 取自片尾窗口 bytes=${snap.data.size - at}")
                    return snap.data.copyOfRange(at.toInt(), snap.data.size)
                }
            }
        }
        // 【273 v31：先登记成窗口再下载】登记之后这一段在缓存里就是"正在下"
        // ⇒ 别的读者（拖动预览的内核、播放内核）会**跟着等**，
        // 而不是自己再去源站下一份 —— 真机 v30：预览那一次自己新建连接，
        // 表头 **2,403 ms**（而窗口里那份 904 ms 后就有了）。
        // 下完照旧 publishBytes ⇒ 精确字节替换这一窗口。
        cache.prefetch(
            cuesAbs,
            cuesAbs + CUES_PROBE_BYTES,
            client,
            OriginWindowCache.TAG_CUES,
            OriginWindowCache.DEFAULT_WAIT_MS,
            replaceTag = OriginWindowCache.TAG_CUES,
        )
        val probe = cache.awaitWindow(OriginWindowCache.TAG_CUES, CUES_WAIT_MS)?.data ?: run {
            NetDiag.log("[cue] cues 窗口没落地 ⇒ 放弃（不预取）")
            return null
        }
        val el = MkvEbml.elementAt(probe, 0)
        if (el == null || el.id != MkvEbml.ID_CUES) {
            NetDiag.log("[cue] $cuesAbs 处不是 Cues ⇒ 放弃（不预取）")
            return null
        }
        if (!el.truncated) {
            publishCues(cuesAbs, probe)
            return probe
        }
        // 【探不够就精确取】元素头里写着内容长度 ⇒ 按它取全。
        val need = el.contentStart + el.contentLength
        if (need <= 0L || need > CUES_MAX_BYTES) {
            NetDiag.log("[cue] cues 声明长度 $need 不合理 ⇒ 放弃（不预取）")
            return null
        }
        val full = fetch(cuesAbs, need) ?: return null
        NetDiag.log("[cue] cues 探得 ${probe.size} 字节不够 ⇒ 按声明长度重取 ${full.size} 字节")
        publishCues(cuesAbs, full)
        return full
    }

    /**
     * 把 cues 本体登记进窗口缓存（273 v28）。
     *
     * 【它消掉哪一跳】内核 seek 时要读**同一段**（MKV 的 cues 表在片尾）。
     * 我们解析索引时已经下过 ⇒ 登记进去 ⇒ 内核那一跳**不碰源站**
     * （真机 v27-a：那一次在源站新建连接，表头 **1,424 ms**）。
     *
     * 【为什么不给它白下一份】同一段字节下两遍 = 白花源站流量，
     * 而源站的带宽是封顶的（真机：并发不涨、反而互相拖慢）。
     */
    private fun publishCues(cuesAbs: Long, bytes: ByteArray) {
        cache.publishBytes(cuesAbs, bytes, OriginWindowCache.TAG_CUES)
    }

    /** 取一段字节（失败 ⇒ null）。 */
    private fun fetch(start: Long, len: Long): ByteArray? {
        val want = len.coerceAtMost(Int.MAX_VALUE.toLong()).toInt()
        val stream = try {
            client.openRange(start, start + want - 1)
        } catch (e: Exception) {
            NetDiag.log("[cue] 取字节失败 ${e.javaClass.simpleName}: ${e.message}")
            return null
        }
        try {
            val buf = ByteArray(want)
            var got = 0
            while (got < buf.size) {
                val n = stream.readInto(buf, got, buf.size - got)
                if (n <= 0) break
                got += n
            }
            return if (got == buf.size) buf else buf.copyOf(got)
        } catch (e: Exception) {
            NetDiag.log("[cue] 读字节失败 ${e.javaClass.simpleName}: ${e.message}")
            return null
        } finally {
            // 读完 ⇒ 连接回池（下次复用，省一次握手）；没读完 ⇒ 断开。
            if (stream.exhausted) stream.finish() else stream.abort()
        }
    }

    companion object {
        /**
         * 自己取片头时的长度（字节）。
         *
         * 【为什么是 64 KiB】SeekHead 与 Info 都在文件开头（真机实测在前几百 KB 内）
         * ⇒ 64 KiB 足够，冷源上 ≈0.3 s 就到。
         */
        const val HEAD_PROBE_BYTES = 64L * 1024L

        /**
         * 探 cues 时的长度（字节）。
         *
         * 【为什么是 256 KiB】真机 cues 元素 ≈130 KB ⇒ 一次探就够，
         * 不用"探 + 精确取"两次往返。
         */
        const val CUES_PROBE_BYTES = 256L * 1024L

        /**
         * cues 声明长度的上限（字节）。
         *
         * 【为什么要有上限】元素头里的长度是**声明值** —— 若它离谱（大到几 GB），
         * 那就是"这个元素不是我要找的" ⇒ 不按它取（否则就是白下几个 GB）。
         */
        const val CUES_MAX_BYTES = 16L * 1024L * 1024L

        /** 等片头窗口落地的上限（毫秒）。 */
        private const val HEAD_WAIT_MS = 2_000L

        /** 等片尾窗口落地的上限（毫秒）。 */
        private const val TAIL_WAIT_MS = 2_000L

        /**
         * 等 cues 窗口落地的上限（毫秒）。
         *
         * 【为什么给 10 秒】cues 在片尾，源站那边是**冷区**
         * （真机 273 v13/v14：0.34 MB/s，比片头慢一个量级）⇒
         * 256 KiB 要好几秒。等它比"自己另开一条连接再下一份"划算：
         * 后者要新建连接（表头 1.4–2.4 s）还多下一份同样的字节。
         */
        private const val CUES_WAIT_MS = 10_000L
    }
}
