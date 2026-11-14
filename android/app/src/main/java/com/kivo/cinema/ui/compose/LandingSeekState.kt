package com.kivo.cinema.ui.compose

/**
 * 273 v42：落点跳转的**状态** —— 谁在等、等到了没有、过期没有。
 *
 * ## 为什么单独一个文件
 *
 * "落点跳转"这件事有两半，变化原因不同：
 *  · **等窗口就绪**（后台线程，阻塞）—— 在 [PlayerActionRouter] 里；
 *  · **这次等待还有效吗**（序号 / 状态）—— 在这里。
 *
 * 而 [PlayerComposeActivity] 已经 329 行、[PlayerActionRouter] 已经 371 行，
 * 两边都**压在铁线上**（≤300 行）⇒ 再往里塞字段只会让两个文件都爆掉。
 *
 * ## 它解决什么
 *
 * 真机 v35-land 遗留：落点那一跳的字节供给已经**完全正确**
 * （内核一次都没回源站，全部由窗口边下边给），但仍剩一次 **499 ms** 的缓冲事件：
 *
 * ```
 * [15.698] stream level seek from 141012675 to 1613145699
 * [16.149] seek done
 * [16.853] first video frame after restart shown
 * [17.005] Enter buffering (100% -> 0%)     ← 首帧之后 demux 缓存空了
 * [17.403] End buffering (waited 0.499 secs)
 * ```
 *
 * 归因：**内核在窗口第一片到达前就跳过去了**（真机：预取发起后 2 ms 就跳，
 * 第一片 348 ms 后才到）⇒ 内核先把 demux 缓存读空 ⇒ 紧接着缓存耗尽 ⇒ 空 0.5 s。
 *
 * 修法：**等第一片就绪再跳**（等整块不行 —— 整块要 2683 ms，比现在更差）。
 */

/**
 * 落点跳转的一次尝试。
 *
 * 【为什么是"尝试"而不是"跳转"】等待期间用户可能又拖了一次 ⇒
 * 旧的那次等待结果必须被丢弃，否则会把用户跳回旧位置。
 */
internal class LandingSeekState {

    /**
     * 当前这次落点跳转的**序号**。
     *
     * 【为什么用序号而不是布尔】序号单调递增 ⇒ 比较相等即可判"是不是同一次"。
     * 布尔需要处理"回到 false 再变 true"的时序，序号不用。
     */
    @Volatile var seq: Long = 0L

    /**
     * 开始一次新的落点跳转（发序号）。
     *
     * @return 本次的序号（等待方拿它判断结果是不是自己的）。
     */
    fun begin(): Long {
        seq += 1L
        return seq
    }

    /**
     * 这次等待的结果还有效吗？
     *
     * @param mySeq 本次等待开始时拿到的序号。
     */
    fun isCurrent(mySeq: Long): Boolean = seq == mySeq
}
