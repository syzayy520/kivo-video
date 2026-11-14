package com.kivo.cinema.net

/**
 * 273 v18/v26：**MKV 索引** —— 把"已知的播放时间"换算成"精确的簇字节偏移"。
 *
 * ## 它解决什么
 * 内核续播时会把播放头 seek 到某个字节偏移（真机：21.063 s → 8,490,718）。
 * 那一跳在起播关键路径上，而中段在源站是**冷的** ⇒ 真机多付 3.2 s。
 * v26 把同一张表用到**拖动落点**上（远跳后 0.6 s 停顿）。
 *
 * 【怎么消掉】片源的 MKV 里有一张 **cues** 表（"时间 → 簇位置"）。
 * 它的位置由片头的 **SeekHead** 给出（"Cues 元素在 Segment 数据起点的第几字节"）。
 *
 * ## 为什么拆成"读片头"与"读 cues"两段（v26）
 * 片头（EBML 头 + SeekHead + Info）与 cues 表**在文件的两个尽头**，
 * 而"片尾窗口"未必盖得住 cues 的**开头**（真机：尾窗 64 KiB，
 * cues 起点比它还早 64,958 字节 ⇒ 盖不住 ⇒ 解析不出来）。
 * ⇒ 两段分开：先从片头算出 cues 的**精确位置**，再按那个位置去取。
 *
 * ## 两个前提
 *  · **Segment 数据起点**：`SeekPosition` 是**相对 Segment 数据起点**的偏移，
 *    不是相对文件头 ⇒ 差一个常数就是差几十 MB（真机：52 字节）。
 *  · **TimecodeScale**：`CueTime` 的单位是刻度，不是毫秒
 *    （真机：1 刻度 = 1,000,000 ns = 1 ms）。
 *
 * ## 失败语义
 * 解析不出来 ⇒ 返回 null ⇒ **不预取**（读侧照常走网络，功能不缺）。
 * 解析错 ⇒ 最多"猜错一个偏移" ⇒ 那一跳不命中而已 —— **绝不喂错字节给内核**。
 */
internal class MkvCueIndex private constructor(
    /** Segment 数据起点（文件绝对偏移）：`SeekPosition` 相对它。 */
    private val segmentDataStart: Long,
    /** 一个刻度等于多少纳秒。 */
    private val timecodeScaleNs: Long,
    /** 索引项（按刻度升序）。 */
    private val entries: List<Entry>,
) {

    /** 一条索引：刻度时间 + 簇位置（**相对** [segmentDataStart]）。 */
    class Entry(val cueTicks: Long, val clusterRel: Long)

    /** 索引项数（证据里用来看"这张表读到了没有"）。 */
    val count: Int get() = entries.size

    /**
     * 这个时间**是否被索引覆盖**（落点不会算错）。
     *
     * 【为什么要有】cues 表可能被缓冲夹住（只读到前半段）⇒
     * 晚于最后一条索引的时间**没有对应簇** ⇒ 拿它去算偏移会算到"最后一个簇"
     * （**错的**）⇒ 那就宁可**不预取**。
     */
    fun covers(timeMs: Long): Boolean {
        if (entries.isEmpty()) return false
        val ticks = timeMs * 1_000_000L / timecodeScaleNs
        return ticks <= entries.last().cueTicks
    }

    /**
     * 时间（毫秒）→ 簇的**文件绝对偏移**。
     *
     * 【语义】取**最后一个** `CueTime ≤ 目标` 的索引项 —— 也就是内核会落到的那个簇。
     *
     * @return 绝对偏移；-1 = 没有可用索引（调用方按"不预取"处理）。
     */
    fun clusterOffsetAt(timeMs: Long): Long {
        if (entries.isEmpty()) return -1L
        val ticks = timeMs * 1_000_000L / timecodeScaleNs
        var idx = -1
        for (i in entries.indices) {
            if (entries[i].cueTicks <= ticks) idx = i else break
        }
        // 目标早于第一条索引 ⇒ 落回第一条（内核也会这么落）。
        if (idx < 0) idx = 0
        return segmentDataStart + entries[idx].clusterRel
    }

    companion object {

        /** 默认时间刻度（1 刻度 = 1 ms）—— MKV 规范值。 */
        const val DEFAULT_TIMECODE_SCALE_NS = 1_000_000L

        /**
         * **片头**解析出来的东西：算 cues 位置要用的三样。
         *
         * @param segmentDataStart Segment 数据起点（文件绝对偏移）。
         * @param timecodeScaleNs 一个刻度等于多少纳秒。
         * @param cuesAbs Cues 元素的**文件绝对偏移**。
         */
        class Head(
            val segmentDataStart: Long,
            val timecodeScaleNs: Long,
            val cuesAbs: Long,
            /**
             * **视频轨的轨道号**；-1 = 片头里没有轨道表（或没有视频轨）。
             *
             * 【为什么需要】一个 CuePoint 里可以有多组 CueTrackPositions（每条轨一组），
             * 它们的 CueClusterPosition **不一样**（视频轨的关键帧更稀、音轨更密）。
             * 取错轨 ⇒ 落点算到别的轨的簇上。
             */
            val videoTrackNumber: Int = -1,
        )

        /**
         * 解析**片头** —— 拿到 cues 的精确位置（这一步不读 cues 本体）。
         *
         * @param head 片头缓冲（必须含 EBML 头 + SeekHead + Info）。
         * @param headStart 片头缓冲在文件里的起始偏移（通常是 0）。
         * @return 片头信息；null = 解析不出来（**不预取**）。
         */
        fun parseHead(head: ByteArray, headStart: Long): Head? {
            val ebml = MkvEbml.elementAt(head, 0) ?: return null
            if (ebml.id != MkvEbml.ID_EBML) return null
            val segment = MkvEbml.elementAt(head, ebml.contentEnd) ?: return null
            if (segment.id != MkvEbml.ID_SEGMENT) return null

            val segmentDataStart = headStart + segment.contentStart
            var timecodeScale = DEFAULT_TIMECODE_SCALE_NS
            var cuesRel = -1L
            var videoTrack = -1
            for (el in MkvEbml.children(head, segment.contentStart, segment.contentEnd)) {
                when (el.id) {
                    MkvEbml.ID_SEEK_HEAD -> {
                        val found = readSeekHead(head, el)
                        if (found >= 0) cuesRel = found
                    }
                    MkvEbml.ID_INFO -> {
                        val scale = readTimecodeScale(head, el)
                        if (scale > 0) timecodeScale = scale
                    }
                    MkvEbml.ID_TRACKS -> {
                        val found = readVideoTrackNumber(head, el)
                        if (found > 0) videoTrack = found
                    }
                }
            }
            if (cuesRel < 0L) return null
            return Head(segmentDataStart, timecodeScale, segmentDataStart + cuesRel, videoTrack)
        }

        /**
         * Tracks → **视频轨的轨道号**（-1 = 没有视频轨）。
         *
         * 【为什么必须按轨取】MKV 的一个 CuePoint 可以带多组 CueTrackPositions
         * （每条轨一组），它们的簇位置**不同**。取到音轨的位置 ⇒
         * 落点算到音轨簇上 ⇒ 那一跳读到的字节不是视频关键帧。
         */
        private fun readVideoTrackNumber(buf: ByteArray, tracks: MkvEbml.Element): Int {
            for (entry in MkvEbml.children(buf, tracks.contentStart, tracks.contentEnd)) {
                if (entry.id != MkvEbml.ID_TRACK_ENTRY) continue
                var number = -1
                var type = -1L
                for (k in MkvEbml.children(buf, entry.contentStart, entry.contentEnd)) {
                    when (k.id) {
                        MkvEbml.ID_TRACK_NUMBER -> {
                            val v = MkvEbml.readUint(buf, k.contentStart, k.contentLength.toInt())
                            if (v > 0) number = v.toInt()
                        }
                        MkvEbml.ID_TRACK_TYPE -> {
                            val v = MkvEbml.readUint(buf, k.contentStart, k.contentLength.toInt())
                            if (v >= 0) type = v
                        }
                    }
                }
                if (type == MkvEbml.TRACK_TYPE_VIDEO && number > 0) return number
            }
            return -1
        }

        /**
         * 解析**cues 本体** —— 缓冲必须从 Cues 元素的**第一个字节**开始。
         *
         * @param cues Cues 元素起始处的缓冲。
         * @param head [parseHead] 的结果。
         * @return 索引；null = 解析不出来（**不预取**）。
         */
        fun parseCues(cues: ByteArray, head: Head): MkvCueIndex? {
            val el = MkvEbml.elementAt(cues, 0) ?: return null
            if (el.id != MkvEbml.ID_CUES) return null
            val entries = readCuePoints(cues, el, head.videoTrackNumber)
            if (entries.isEmpty()) return null
            return MkvCueIndex(head.segmentDataStart, head.timecodeScaleNs, entries)
        }

        /**
         * SeekHead → Cues 的**相对**位置。
         *
         * 【为什么不用文件绝对偏移】MKV 规范：SeekPosition 相对 **Segment 数据起点**。
         */
        private fun readSeekHead(buf: ByteArray, head: MkvEbml.Element): Long {
            var cuesRel = -1L
            for (seek in MkvEbml.children(buf, head.contentStart, head.contentEnd)) {
                if (seek.id != MkvEbml.ID_SEEK) continue
                var seekId = -1
                var seekPos = -1L
                for (k in MkvEbml.children(buf, seek.contentStart, seek.contentEnd)) {
                    when (k.id) {
                        MkvEbml.ID_SEEK_ID -> seekId = MkvEbml.readId(buf, k.contentStart)
                        MkvEbml.ID_SEEK_POSITION -> {
                            val v = MkvEbml.readUint(buf, k.contentStart, k.contentLength.toInt())
                            if (v >= 0L) seekPos = v
                        }
                    }
                }
                if (seekId == MkvEbml.ID_CUES && seekPos >= 0L) cuesRel = seekPos
            }
            return cuesRel
        }

        /** Info → TimecodeScale（读不到就用规范默认值 1 ms）。 */
        private fun readTimecodeScale(buf: ByteArray, info: MkvEbml.Element): Long {
            for (k in MkvEbml.children(buf, info.contentStart, info.contentEnd)) {
                if (k.id != MkvEbml.ID_TIMECODE_SCALE) continue
                val v = MkvEbml.readUint(buf, k.contentStart, k.contentLength.toInt())
                if (v > 0L) return v
            }
            return -1L
        }

        /**
         * Cues → 索引项。
         *
         * 【只收"读全了"的 CuePoint】窗口可能把 Cues 截断（尾窗只装了前半段）。
         * 截断的那条 ⇒ 它的簇位置读不全 ⇒ 跳过（宁可少一条，不可猜错）。
         *
         * 【按轨取】优先取**视频轨**那一组的簇位置（[videoTrack]）；
         * 视频轨号不知道（-1）或这一组里没有视频轨 ⇒ 退回"取最后一个位置"。
         */
        private fun readCuePoints(
            buf: ByteArray,
            cues: MkvEbml.Element,
            videoTrack: Int,
        ): List<Entry> {
            val out = ArrayList<Entry>()
            for (cp in MkvEbml.children(buf, cues.contentStart, cues.contentEnd)) {
                if (cp.id != MkvEbml.ID_CUE_POINT || cp.truncated) continue
                var cueTicks = -1L
                var clusterRel = -1L
                var fallbackRel = -1L
                for (k in MkvEbml.children(buf, cp.contentStart, cp.contentEnd)) {
                    when (k.id) {
                        MkvEbml.ID_CUE_TIME -> {
                            val v = MkvEbml.readUint(buf, k.contentStart, k.contentLength.toInt())
                            if (v >= 0L) cueTicks = v
                        }
                        MkvEbml.ID_CUE_TRACK_POSITIONS -> {
                            var posTrack = -1
                            var posRel = -1L
                            for (p in MkvEbml.children(buf, k.contentStart, k.contentEnd)) {
                                when (p.id) {
                                    MkvEbml.ID_CUE_TRACK -> {
                                        val v = MkvEbml.readUint(buf, p.contentStart, p.contentLength.toInt())
                                        if (v > 0) posTrack = v.toInt()
                                    }
                                    MkvEbml.ID_CUE_CLUSTER_POSITION -> {
                                        val v = MkvEbml.readUint(buf, p.contentStart, p.contentLength.toInt())
                                        if (v >= 0L) posRel = v
                                    }
                                }
                            }
                            if (posRel < 0L) continue
                            if (videoTrack > 0 && posTrack == videoTrack) clusterRel = posRel
                            else fallbackRel = posRel
                        }
                    }
                }
                if (clusterRel < 0L) clusterRel = fallbackRel
                if (cueTicks >= 0L && clusterRel >= 0L) out.add(Entry(cueTicks, clusterRel))
            }
            out.sortBy { it.cueTicks }
            return out
        }
    }
}
