package com.kivo.cinema.playback

/**
 * 280/301 · **网络恢复耗时**（275 §6.1 的 `network_recovery_ms`）。
 *
 * ## 275 的定义
 * 「网络恢复 → 媒体时钟再次前进」。注意**不是**"网络恢复"本身 ——
 * 网络回来但内核还在读缓存/重建连接时，用户看到的仍然是卡住。
 *
 * ## 状态机
 * ```
 * 位置在走 ──(位置停 ≥3s 且系统报离线)──> 离线中
 *    ↑                                      │
 *    └──(位置重新增长，记 recovery)─────── 网络回来了（记 onlineAt）
 * ```
 *
 * ## 为什么要三道闸门，不能只看"离线/在线"
 * 1. **位置停**：必须真的停了（≥[STALL_MS]），否则正常帧间隔/seek 抖动都会误触发；
 * 2. **系统报离线**：位置停可能是暂停/解码慢/seek，不一定是网；
 * 3. **位置重新增长**：网络回来不等于时钟回来（可能还在等表头）。
 *
 * 三条同时成立才记一次恢复 —— 少一条就会把"暂停 5 秒"记成"断网恢复 5 秒"。
 *
 * ## 为什么不自己查网络
 * 本类**不碰 Android API**（纯状态机，可单测）。调用方每次采样把
 * `online` 传进来（`NetworkLinkMonitor.isOnline` 是静态查询，不注册回调）。
 *
 * ## 边界
 * · 只在**播放中**有意义：暂停时调用方不要喂（`paused=true` 会清状态）；
 * · 采样周期是 [com.kivo.cinema.playback.MpvCacheDiag] 的 2 s
 *   ⇒ 恢复耗时的分辨率约 ±2 s，比 P253 手工测（2.0–2.2 s）粗；
 * · 一次会话只记**最近一次**恢复（够回答"网络回来多久能继续"）。
 *
 * 【为什么不是 internal】`androidTest` 是独立模块，看不到 app 的 internal
 * ⇒ 想用 instrumentation 验证状态机，必须是 public。纯状态机，公开无副作用。
 */
object NetworkRecoveryWatch {

    /** 位置停多久才算"真停了"（毫秒）。低于它就只是正常播放抖动。 */
    private const val STALL_MS = 3_000L

    /** 判定位置"在增长"的最小步进（秒）。2 s 采样下正常播放至少前进 1.5 s。 */
    private const val POS_EPSILON_SEC = 0.05

    private var lastPosSec = -1.0
    private var stallSinceMs = 0L
    private var offlineAtMs = 0L
    private var onlineAtMs = 0L

    @Volatile
    private var lastRecoveryMs = -1L

    /** 最近一次"网络恢复 → 时钟前进"的耗时；-1 = 本次会话没发生过。 */
    fun lastRecoveryMs(): Long = lastRecoveryMs

    @Synchronized
    fun reset() {
        lastPosSec = -1.0
        stallSinceMs = 0L
        offlineAtMs = 0L
        onlineAtMs = 0L
        lastRecoveryMs = -1L
    }

    /**
     * 喂一次采样。
     *
     * @param positionSec 当前媒体时钟（秒）；<0 表示读不到（本类忽略这一拍）
     * @param paused 会话是否处于暂停（暂停时位置本就不动，不能当停滞）
     * @param online 当前是否有可用网络（局域网判据见 NetworkLinkMonitor）
     * @param nowMs 单调时钟（`SystemClock.elapsedRealtime()`）
     */
    @Synchronized
    fun note(positionSec: Double, paused: Boolean, online: Boolean, nowMs: Long) {
        if (positionSec < 0.0) return

        // 暂停期间位置本来就不动 —— 清掉停滞计数，别把"暂停"记成"断网"。
        if (paused) {
            stallSinceMs = 0L
            lastPosSec = positionSec
            return
        }

        val growing = lastPosSec >= 0.0 && positionSec - lastPosSec > POS_EPSILON_SEC
        val firstSample = lastPosSec < 0.0
        lastPosSec = positionSec

        if (growing) {
            // 时钟回来了。如果之前记过 onlineAt（网络已恢复但还没继续），这里就是恢复点。
            if (onlineAtMs > 0L) {
                lastRecoveryMs = nowMs - onlineAtMs
                com.kivo.cinema.net.NetDiag.log(
                    "[recovery] 网络恢复 → 媒体时钟前进 ${lastRecoveryMs}ms" +
                        "（离线 ${nowMs - offlineAtMs}ms）",
                )
                onlineAtMs = 0L
                offlineAtMs = 0L
            }
            stallSinceMs = 0L
            return
        }

        if (firstSample) return

        // 位置没增长：先记停滞起点，够久且系统报离线才认定"断网"。
        if (stallSinceMs == 0L) stallSinceMs = nowMs
        val stalledFor = nowMs - stallSinceMs
        if (stalledFor >= STALL_MS && !online && offlineAtMs == 0L) {
            offlineAtMs = nowMs
            com.kivo.cinema.net.NetDiag.log("[recovery] 判定离线（位置已停 ${stalledFor}ms）")
        }

        // 已确认离线，网络回来了 ⇒ 记 onlineAt，开始等时钟前进。
        if (offlineAtMs > 0L && onlineAtMs == 0L && online) {
            onlineAtMs = nowMs
            com.kivo.cinema.net.NetDiag.log("[recovery] 网络已恢复，开始等媒体时钟前进")
        }
    }
}
