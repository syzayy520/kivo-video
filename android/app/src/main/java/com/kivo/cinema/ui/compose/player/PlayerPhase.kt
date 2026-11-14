package com.kivo.cinema.ui.compose.player

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

/**
 * 播放阶段（**唯一真值模型**）。
 *
 * ## 为什么要有它
 * 272 之前，播放页的"缓冲 / 失败"是靠 `statusText` 的**文案子串**判定的：
 * `statusText.contains("缓冲")`、`contains("失败")`、`contains("中断")`。
 * 那是**拿文案当状态**，后果实测有三处：
 * · 缓冲态恒 false（没有一条 statusText 真含"缓冲"）⇒ 中央波纹是死代码；
 * · "这个片源无法播放""音频无法启动"等致命错误不含"失败"⇒ 用户只见黑屏；
 * · "网络中断，正在重连…"含"中断"⇒ 自动重连时弹出居中「播放失败」。
 *
 * ## 铁律
 * · 阶段只从**内核真值**来：会话 `state`、`buffering` 标志、`lastError`、`isRunning()`。
 * · [Failed.reason] **必须非空**：空串等于没告诉用户原因，比不显示更糟。
 * · `statusText` 降级为纯展示文案（调试 / 日志），**不再参与任何判定**。
 */
sealed interface PlayerPhase {

    /** 未起播（还没拿到面，或刚重试）。 */
    data object Idle : PlayerPhase

    /** 正常播放中。 */
    data object Playing : PlayerPhase

    /** 起播中 / 卡缓冲：内核还活着，但在等数据。 */
    data object Buffering : PlayerPhase

    /**
     * 网络中断，正在重连 —— **不是失败**。
     * 与 [Failed] 分开是因为两者的用户动作完全不同：重连只需要等待，
     * 失败需要用户决定重试还是退出。
     */
    data object Reconnecting : PlayerPhase

    /** 播完（到片尾）。 */
    data object Ended : PlayerPhase

    /** 致命错误：[reason] 是给用户看的非空原因。 */
    data class Failed(val reason: String) : PlayerPhase
}

/** 失败原因（空串视作没有原因，不给用户看）。 */
val PlayerPhase.failureReason: String?
    get() = (this as? PlayerPhase.Failed)?.reason?.takeIf { it.isNotBlank() }

/**
 * 阶段持有者：阶段值 + 进入时刻（看门狗要用）。
 *
 * 【为什么单独一个类】换阶段要**同时**刷新两样东西：阶段本身和"何时进的"。
 * 放在 Activity 里就是两个可变字段，谁改阶段都得记得改时间戳 —— 漏一次，
 * 看门狗就会拿旧时间戳判超时（假失败）。绑在一起就没有漏的余地。
 *
 * @param onBuffering 进入缓冲态时的回调（起播看门狗在这里挂上）。
 */
internal class PlayerPhaseHolder(private val onBuffering: () -> Unit) {

    /**
     * 【为什么不用 `by mutableStateOf(...)` + 自定义 setter】
     * Kotlin 不允许委托属性带非默认的访问器实现（`field` 只对默认实现可见），
     * 编译期直接报 "Delegated property cannot have accessors with non-default
     * implementations"。所以底层用 `MutableState` 手写读写 ——
     * 行为不变（写值触发重组），但换阶段时能顺带刷新进入时刻。
     */
    private val state = mutableStateOf<PlayerPhase>(PlayerPhase.Idle)

    var phase: PlayerPhase
        get() = state.value
        set(value) {
            state.value = value
            enteredMs = System.currentTimeMillis()
            if (value is PlayerPhase.Buffering) onBuffering()
        }

    /** 当前阶段进入的时刻（看门狗用；换阶段即刷新）。 */
    var enteredMs = 0L
}
