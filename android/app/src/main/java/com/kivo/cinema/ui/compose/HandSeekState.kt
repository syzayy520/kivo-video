package com.kivo.cinema.ui.compose

/**
 * 全屏横滑 seek 的**一次进行中的手势**状态（B4-T3 / 审计 DUT-01）。
 *
 * ## 为什么不再是顶层 `private var`
 * 旧实现把 `hSeekStartMs/hSeekAccumMs/hSeekTargetMs/hSeekLastAt` 写成
 * `PlayerGestureRouter.kt` 的**顶层私有变量** ⇒ 它们属于**进程**，不随播放页生死：
 * - 上一次会话没走完 `onHorizontalDragEnd`，`startMs` 残留非零 ⇒
 *   新会话第一次横滑从**旧起点**算，松手跳到上次的位置；
 * - 若上次走完了（`startMs` 归零），`onHorizontalDrag` 的
 *   `if (startMs <= 0L) return` 会把**第一次横滑吞掉**（用户感知："第一次横滑没反应"）。
 *
 * 现在由调用方 `remember { HandSeekState() }` 持有：Composition 销毁即整份丢弃，
 * 第二次进播放器一定是干净的 1.0 起点。
 *
 * ## 为什么不用 `mutableStateOf`
 * 四个字段只在 pointerInput 回调里读写，**从不参与重组**。用快照态等于每帧付一次
 * Snapshot 写入与失效检查（MOT-08 记过同类浪费），而这里的语义需求是"纯计算中间量"。
 * 真正驱动 UI 的是共享的 `scrub`（PlayerScrubState）与 `previewMs`，它们不变。
 *
 * ## 为什么不做全局单例
 * 单例就是旧 bug 的另一种写法。若将来有第二个调用方（例如桌面端或测试），
 * 显式传参即可 —— 状态随持有者生死，读代码时能一眼看出边界。
 */
internal class HandSeekState {

    /**
     * 是否正在进行横滑。
     *
     * 【为什么不用 startMs==0 当哨兵】片头 `positionMs=0` 时 `begin(0)` 会让起点真是 0，
     * 旧的 `if (startMs <= 0) return` 会把整次片头横滑吞掉。用独立布尔标记后，
     * 起点即使是 0 也能正常横滑；`clear()` 才置 false。
     */
    var active: Boolean = false
        private set

    /** 本次横滑的起点（毫秒）。0 是合法值（片头），不再承担"是否进行中"的语义。 */
    var startMs: Long = 0L

    /** 累计位移换算出的时长偏移（负 = 后退）。 */
    var accumMs: Long = 0L

    /** 当前目标时刻（毫秒）；松手 commit 时以它为准。 */
    var targetMs: Long = 0L

    /** 实时 seek 的节流锚点（`SystemClock.uptimeMillis`）。 */
    var lastAt: Long = 0L

    /** 开始一次新横滑：把上一次可能留下的残留一次性清干净，再记新起点。 */
    fun begin(fromMs: Long) {
        active = true
        startMs = fromMs
        accumMs = 0L
        targetMs = fromMs
        lastAt = 0L
    }

    /** 结束（或取消）：置 inactive 并归零，下次 `begin` 从干净状态起。 */
    fun clear() {
        active = false
        startMs = 0L
        accumMs = 0L
        targetMs = 0L
        lastAt = 0L
    }
}