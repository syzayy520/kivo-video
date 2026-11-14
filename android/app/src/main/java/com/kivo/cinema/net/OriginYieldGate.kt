package com.kivo.cinema.net

/**
 * 273 v29：**源站让路闸** —— 让某一路取数在一段时间里主动"让路"。
 *
 * ## 它解决什么
 *
 * 真机实测（Node 探针，源站 emby.bangumi.ca）：源站带宽是**封顶**的，
 * 而并发会互相踩 —— 同一段 4 MiB，单独下要 **1,650 ms**，
 * 同时有 3 条传输在跑时要 **3,302 ms**（≈2 倍）。
 *
 * 拖动时占着带宽的不止落点那一跳：
 *  · 播放主链路的预读线程还在下**用户已经放弃的那个位置**（4 MiB 一块）；
 *  · 保温线程每 8 s 发一个 1 字节请求，运气不好会撞上落点那一跳。
 *
 * 于是落点那一跳只拿到"剩下的"带宽 ⇒ 真机实测落点表头 284–2,905 ms、
 * 落点字节要 1.6–3.3 s 才下完 ⇒ 拖动之后白屏 2–4 秒。
 *
 * ## 做法
 *
 * 拖动期间打开闸门：主链路的预读线程**不开新块、不做 carry 读**，
 * 保温线程**不发 ping** ⇒ 落点那一跳独占源站的连接与带宽。
 *
 * ## 闸门只让路，不饿死播放
 *
 * 让路的条件是"环里还有足够缓冲"（≥ 容量的 1/4）——
 * 缓冲见底时闸门自动失效，播放永远优先。
 * 新开的读者（拖动落点后新建的那条）环里是空的 ⇒ 它从不让路。
 */
internal class OriginYieldGate {

    /** 让路到什么时候（单调时钟，毫秒）。 */
    @Volatile private var untilMs = 0L

    /** 现在是不是在让路。 */
    fun isYielding(): Boolean = android.os.SystemClock.elapsedRealtime() < untilMs

    /** 让路 [ms] 毫秒（重复调用只**延长**，不缩短）。 */
    fun yieldFor(ms: Long) {
        val until = android.os.SystemClock.elapsedRealtime() + ms
        if (until > untilMs) untilMs = until
    }

    /** 立刻恢复（不缩短到过去，只在必要时用）。 */
    fun release() {
        untilMs = 0L
    }
}
