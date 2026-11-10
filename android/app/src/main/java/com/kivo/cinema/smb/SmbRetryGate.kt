package com.kivo.cinema.smb

/**
 * 重连退避状态机（250 D1）。纯逻辑：不联网、不碰 Android、不碰 libsmb2。
 *
 * 从 [SmbByteSource] 里剥出来的理由不是"行数"，是**它有一个独立的失败模式**：
 * 243 的 r1 版本把"重连失败"当终态，于是断网 8 秒、网络第 3 秒就恢复，
 * 剩下 5 秒的读全落在一个已经死掉的判断上。那之后改成"退避后持续重试"，
 * 逻辑对了，但和读取纠缠在一起，没法单独证明它对。
 *
 * 现在它只回答两个问题：**现在能不能再试**、**不能试该说什么**。
 * 判据可以写成一张时间表（见 `SmbRetryGateTest`），不需要真机。
 *
 * 退避序列 400 → 800 → 1600 → 3200 → 4000 封顶，沿用 243 的真机验证值，
 * 本卡不改数字。
 */
class SmbRetryGate(
    /** 链路是否可达。注入而不是自己查系统：这一层要能在 JVM 上跑。 */
    private val online: () -> Boolean = { true },
    private val baseMs: Long = BASE_MS,
    private val capMs: Long = CAP_MS,
) {
    /** 当前退避长度；0 表示"没有在退避中"。 */
    var backoffMs: Long = 0L
        private set

    /** 下一次允许尝试的时刻（毫秒，调用方给时基）。 */
    var nextAttemptAtMs: Long = 0L
        private set

    /** 上一次失败的原因，用于给用户一句准确的话。 */
    var lastFailure: String? = null
        private set

    private var wasOffline = false

    /**
     * 现在能不能发起一次读。
     *
     * **有意带副作用**：从"离线"回到"在线"的那一次调用会把退避清零。
     * 不做这件事的话，网络恢复后还要白等最多 4 秒才重连 —— 用户看到的是
     * "网回来了但画面还停着"，正是 245 要消灭的观感。
     */
    fun canAttempt(nowMs: Long): Boolean {
        if (!online()) {
            wasOffline = true
            return false
        }
        if (wasOffline) {
            wasOffline = false
            backoffMs = 0L
            nextAttemptAtMs = 0L
        }
        return nowMs >= nextAttemptAtMs
    }

    /** 读成功：退避与失败原因一起清掉。 */
    fun noteSuccess() {
        backoffMs = 0L
        nextAttemptAtMs = 0L
        lastFailure = null
    }

    /**
     * 读失败：登记原因并推迟下一次尝试。
     *
     * 离线时**不翻倍**。网络已经没了，指数退避只会让"网回来"之后的那一次
     * 尝试也被推迟 —— 指数退避是用来保护服务器的，不是用来惩罚用户的。
     */
    fun noteFailure(nowMs: Long, reason: String? = null) {
        lastFailure = reason
        if (!online()) {
            nextAttemptAtMs = nowMs
            return
        }
        backoffMs = if (backoffMs <= 0L) baseMs else minOf(backoffMs * 2, capMs)
        nextAttemptAtMs = nowMs + backoffMs
    }

    /** 给 [java.io.IOException] 用的一句话。优先说真实原因，其次才说"正在等"。 */
    fun denyReason(): String = when {
        !online() -> OFFLINE_REASON
        else -> lastFailure?.takeIf { it.isNotBlank() } ?: WAITING_REASON
    }

    companion object {
        const val BASE_MS = 400L
        const val CAP_MS = 4_000L
        const val OFFLINE_REASON = "网络已断开"
        const val WAITING_REASON = "SMB 连接已断开，正在等待重连"
    }
}
