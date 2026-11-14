package com.kivo.cinema.net

import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.atomic.AtomicReference

/**
 * 273 v44：延迟的**滑动统计**（保留最近 N 个样本，给出 p90）。
 *
 * ## 它为什么独立成一个文件
 *
 * "记一个样本"与"怎么算分位数"是**两种变化原因**：
 * 改采样窗口不该动算法，改算法不该动采样点。
 * 而且这个统计**不只服务于表头耗时**（连接建立耗时、窗口下载耗时都能用）
 * ⇒ 做成通用件，不绑死在某一种耗时上。
 *
 * ## 为什么用 p90 而不是平均 / 最大
 *
 * | 统计量 | 问题 |
 * | --- | --- |
 * | 平均 | 被偶发离群值拉高（源站一次 TLS 抖动就能拖垮） |
 * | 最大 | 一次抖动把上限顶到几秒 ⇒ 用户感觉"按了没反应" |
 * | **p90** | **90% 的请求都快**，只有最慢的 10% 拉高上限 ⇒ 覆盖常态又不被绑架 |
 *
 * ## 线程安全
 *
 * 采样发生在**多个连接线程**（预读 / 保温 / 落点窗口各一条），
 * 读取发生在**决策线程**（落点等待）⇒ 用 [AtomicReference] 整体换数组，
 * 读侧拿到的一定是完整快照（不会读到"写了一半"的数组）。
 */
internal class OriginLatencyStats(
    /** 保留最近多少个样本。 */
    private val capacity: Int = DEFAULT_CAPACITY,
) {

    /**
     * 最近 N 个样本（旧的在前，新的在后）。
     *
     * 【为什么整体替换而不是加锁写】见类注释的线程安全说明。
     */
    private val samples = AtomicReference(LongArray(0))

    /** 已写入的样本总数（用来算"写满了没有"）。 */
    private val total = AtomicLong(0L)

    /**
     * 记一个样本。
     *
     * @param ms 本次耗时（毫秒）。
     */
    fun note(ms: Long) {
        if (ms < 0L) return
        val old = samples.get()
        val next = if (old.size < capacity) {
            LongArray(old.size + 1).also { System.arraycopy(old, 0, it, 0, old.size); it[old.size] = ms }
        } else {
            // 满了 ⇒ 丢掉最老的（滑动窗口）。拷贝一次而不是原地改：原地改会与并发读撞车。
            LongArray(capacity).also {
                System.arraycopy(old, 0, it, 0, capacity - 1)
                it[capacity - 1] = ms
            }
        }
        samples.set(next)
        total.incrementAndGet()    }

    /**
     * 当前 p90（毫秒）。
     *
     * @return **-1 = 还没有样本**（调用方要按"没有基线"处理，不能当成 0）。
     */
    fun p90(): Long {
        val s = samples.get()
        if (s.isEmpty()) return -1L
        val sorted = s.copyOf()
        sorted.sort()
        // p90 的位置：ceil(0.9 * n) - 1（下标从 0 起）。
        val idx = ((sorted.size * 90 + 99) / 100 - 1).coerceIn(0, sorted.size - 1)
        return sorted[idx]
    }

    /** 当前样本数。 */
    fun count(): Int = samples.get().size

    companion object {
        /** 保留最近多少个样本。 */
        const val DEFAULT_CAPACITY = 16
    }
}
