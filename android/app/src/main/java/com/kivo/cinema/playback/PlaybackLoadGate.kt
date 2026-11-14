package com.kivo.cinema.playback

/**
 * 起播的**放行闸门**（273 v15）：内核**建好之后、loadFile 之前**等它放行。
 *
 * ## 为什么要有闸门
 * 真机实测（273 v15，emby.bangumi.ca）：源站每条**新建**连接要 0.8–1.6 s
 * （TLS 握手），**复用**只要 0.3 s。内核起播第一跳（片头）如果撞上
 * "连接还没建好"，那一跳就要付一次握手 —— 真机实测表头 1155 ms。
 *
 * ## 为什么闸门挡的是 loadFile 而不是"起播"
 * 起播有三段：**内核创建**（真机 39 ms）→ **等连接** → **loadFile**（发第一跳）。
 * 把闸门放在"起播之前"（旧做法）会让内核创建也跟着往后挪，
 * 白等 1.3 s；放在 loadFile 之前，内核创建与握手**并行**，
 * 端到端省下同样的 1.3 s（真机：tap→首帧 4.36 s → 3.5 s）。
 *
 * ## 闸门是"加速器"，不是"依赖"
 * 超时（[awaitReady] 返回 false）时照样放行：读侧自己会撞上源站的错误并报错，
 * 卡死起播比慢 0.3 s 更糟。
 */
interface PlaybackLoadGate {

    /**
     * 等放行。
     *
     * @return true = 已放行；false = 超时（**照样放行**，不当失败）。
     */
    fun awaitReady(timeoutMs: Long): Boolean

    /**
     * 闸门的判决（null = 没有判决）。
     *
     * 【什么算判决】源站**明确拒绝**（401/403/404）——
     * 这些是"再等也没用"的状态。瞬时 IO 错误不算判决。
     */
    val verdict: Throwable?
}

/**
 * 源站**明确拒绝**（401/403/404）。
 *
 * 【为什么要单独一类】它的原因要**原样**透给用户，
 * 不能包在"内核播放准备失败（…）"里 —— 那一层会让用户以为是内核坏了。
 */
internal class PlaybackRejected(reason: String) : Exception(reason)

