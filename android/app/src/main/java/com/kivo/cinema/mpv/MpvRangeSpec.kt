package com.kivo.cinema.mpv

/**
 * 273 v15：Range 头的**语义** —— "内核要哪一段"。
 *
 * 【为什么从 [LocalRangeResponder] 里拆出来】
 * 代理要回答两个不同的问题：
 *  · **范围语义**：内核给的 `Range: bytes=A-B` 是什么意思？（这里）
 *  · **供数策略**：这一段怎么读、怎么写、失败怎么办？（[LocalRangeResponder]）
 * 前者会因为"内核换 Range 语义"而变，后者会因为"取数策略变"而变 ——
 * 两种变化原因不该挤在一个文件里（改一个必然蹭到另一个）。
 *
 * 【为什么不依赖总长】
 * 起点/终点完全由请求头决定；总长只用来**夹**终点和判 416 ——
 * 那是代理拿到总长之后的事，不该混进"解析请求头"这一步。
 */
internal data class MpvRangeSpec(
    /** 起点（字节偏移）。 */
    val start: Long,
    /** 终点（**含**）；null = 开口区间（一直到文件尾）。 */
    val endInclusive: Long?,
    /** 内核是不是给了分段请求（决定回 206 还是 200）。 */
    val partial: Boolean,
) {
    companion object {
        /**
         * 解析 Range 头。
         *
         * 【兼容旧行为】`bytes=-N`（后缀范围）内核没发过；
         * 旧实现把它当成 `bytes=0-` 处理，这里保持一致，不引入新语义。
         */
        fun parse(rangeHeader: String?): MpvRangeSpec {
            if (rangeHeader == null || !rangeHeader.startsWith("bytes=")) {
                return MpvRangeSpec(0L, null, false)
            }
            val spec = rangeHeader.removePrefix("bytes=").trim()
            val dash = spec.indexOf('-')
            if (dash <= 0) return MpvRangeSpec(0L, null, false)
            val from = spec.substring(0, dash).trim()
            val start = if (from.isEmpty()) 0L else from.toLongOrNull() ?: 0L
            val to = spec.substring(dash + 1).trim()
            val end = if (to.isEmpty()) null else to.toLongOrNull()
            return MpvRangeSpec(start, end, true)
        }
    }
}
