package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.NetDiag

/**
 * 292：**起播预缓冲闸门** —— 给首帧一个起始 runway，消除起播期连环 buffering。
 *
 * ## 为什么需要它（真机 p291-night-both + 本地装置确定性复现）
 *
 * `MpvFirstFrameWait` 一确认画面上屏就返回，此刻 demuxer 前向 runway≈0
 * （p291-night 两臂首帧都是 `dur=0.0s underrun=true`）。主链路又是**串行补块**：
 * 一块 4 MiB 读完 → 归还连接 → 开下一块付一次表头（公网实测 1.5–8 s，
 * 本地装置 `--chunk-hdr-ms` 可精确注入），这段窗口供给为 0。
 * 首帧零缓冲起步 ⇒ 第一块空窗就直接穿仓，之后靠每块 83 KB/s 的净余量
 * 恢复极慢，形成锯齿下行（本地装置复现：+13.9s 首次 underrun，连环 2 次 / 4.2s）。
 *
 * ## ★ 方案 A 已被 A/B 证伪（292 第一轮）
 *
 * 把 `cache-pause-wait` 从默认 1 s 拉到 6 s（播放中压力期保持）：
 *   · off(wait=1)：2 次 / 总 4.2 s / 最长 2.3 s
 *   · on (wait=6)：1 次 / 总 **8.2 s** / 最长 **8.2 s**
 * underrun 后是**边消费边攒**，净增只有 (供给-码率)，攒够 6 s 要 8 s
 * ⇒ 把连环短冻结合并成一次长冻结。**拉高恢复阈值 = 负优化。**
 *
 * ## ★★ 方案 B 的失败机制（292 第二轮，真机 kernel 取证）
 *
 * 曾把 `cache-pause-initial=yes` 绑在 `onFirstFrame` 上"首帧后立即恢复默认"。
 * 结果 4 臂全无效。kernel 时间线说明原因：
 * ```
 * [4.167] first video frame after restart shown   ← 首帧上屏
 * [4.168] Enter buffering (100% -> 27%)          ← initial 预缓冲**这时候才开始**
 * [4.220] Set property: cache-pause-initial="no" ← onFirstFrame 立刻写回默认
 * [4.220] End buffering (waited 0.05s)           ← 刚起步就被掐断
 * ```
 * mpv 的 initial buffering 是在**首帧显示之后**才进入的（playloop 的
 * `!restart_complete` 分支），把恢复绑首帧 = 亲手打断自己的预缓冲。
 *
 * ## 方案 C（本实现）：等 initial buffering 自然攒够再恢复
 *
 * 起播前无消费，纯攒 `wait` 秒数据只需 `wait × 码率 / 供给`
 * （本地装置 614KB/s、片源 327KB/s ⇒ 攒 4 s 约 2 s），远快于边播边攒。
 *  · loadFile 之前：`cache-pause-initial=yes` + `cache-pause-wait=<N>`；
 *  · [MpvCacheDiag] 每 2 s 喂 runway；**runway ≥ N 才写回**默认
 *    （initial buffering 攒够 N 秒才自然结束 ⇒ 这是可靠的"已完成"信号）；
 *  · [ARM_TIMEOUT_MS] 超时兜底无论如何写回 ⇒ 慢源站也不会永久影响 seek。
 *
 * 写回 `no`/`1.0` 后，seek 重启与播放中偶发 underrun 都回到 291 已验证行为。
 *
 * ## ★ 状态寿命（292 第三轮真机踩坑，别再犯）
 *
 * [beforeLoad] 每次起播都会重设 `armedAtMs`/`restored`，**不需要**跨会话清理。
 * 尤其**不要**在 [MpvCacheDiag.stop] 里调 reset：attach() 开头必先调 stop()，
 * 而它在 [beforeLoad] **之后**（applyPlaybackPolicy → loadFile → attach）⇒
 * 一旦清零，`note()` 里 `now - 0 > 12s` 恒真，第一次采样就「超时兜底」写回，
 * 方案 C 当场退化成方案 B（真机日志：`超时兜底(12s)` 出现在 +4.8s，
 * kernel 里 `cache-pause-initial=no` 又落在首帧同一 4.191s）。
 *
 * 两项都是 playloop 每帧实时读 `opts->` 的普通 option，运行时可改
 * （真机 `Set property: ... -> 1`），且属内部 `paused_for_cache`，
 * **不动 `pause` property** ⇒ UI 的 `isPlaying` 零变化（铁律：不许写 UI）。
 *
 * ## 开关（opt-in，默认关闭）
 *
 * 292 三轮 A/B 证明本策略**无实质收益**（bf 次数不减、最长略增，
 * 只把首次穿仓平移约 4s）⇒ **默认不进真实会话**，仅实验臂显式开启。
 * `debug.kivo.rebuffer=on` ⇒ 启用；缺省 / `off` ⇒ 完全不动作，既有行为一字不变。
 * `debug.kivo.rebufferWait <秒>` 覆盖起播攒多少（默认 [DEFAULT_WAIT_S]）。
 * `debug.kivo.rebufferInitial no` ⇒ 只设 wait、不设 initial（分离变量）。
 * 每次 `setProperty` 成败都留 `[rebuffer]` 日志 —— 不许静默降级。
 */
internal object RebufferPolicy {

    private const val PROPERTY = "debug.kivo.rebuffer"
    private const val WAIT_PROPERTY = "debug.kivo.rebufferWait"
    private const val INITIAL_PROPERTY = "debug.kivo.rebufferInitial"

    /** 起播要攒的前向 runway（秒）。够跨过一次「表头空窗 + 补块」窗口即可。 */
    private const val DEFAULT_WAIT_S = 4.0

    /** 首帧后恢复成 mpv 默认值（options.c: cache_pause_wait = 1.0）。 */
    private const val RELAXED_WAIT_S = 1.0

    /** 兜底：超过这么久仍未攒够就写回默认，不能让 initial 一直影响后续 seek。 */
    private const val ARM_TIMEOUT_MS = 12_000L

    private val enabled: Boolean = prop(PROPERTY)?.trim()?.lowercase() == "on"

    private val waitS: Double =
        prop(WAIT_PROPERTY)?.trim()?.toDoubleOrNull()
            ?.takeIf { it in 1.0..20.0 } ?: DEFAULT_WAIT_S

    private val useInitial: Boolean = prop(INITIAL_PROPERTY)?.trim()?.lowercase() != "no"

    private val lock = Object()

    @Volatile private var armedAtMs = 0L

    @Volatile private var restored = false

    init {
        if (!enabled) NetDiag.log("[rebuffer] 默认关闭（opt-in）：不下发任何起播缓冲选项")
    }

    /**
     * loadFile 之前调用一次：让 mpv 在**开始连续播放前**先进入 initial buffering
     * 攒够 [waitS] 秒前向数据。这是给首帧一个起始 runway 的关键一步。
     */
    fun beforeLoad(native: MpvSession) {
        if (!enabled) return
        armedAtMs = System.currentTimeMillis()
        restored = false
        if (useInitial) {
            val ok = native.setProperty("cache-pause-initial", "yes")
            NetDiag.log("[rebuffer] 起播预置 cache-pause-initial=yes 写入=$ok")
            if (!ok) NetDiag.log("[rebuffer] ⚠ initial 运行时写不进 ⇒ 本臂退化为只调 wait")
        } else {
            NetDiag.log("[rebuffer] 分离变量臂：不下发 cache-pause-initial")
        }
        val ok = native.setProperty("cache-pause-wait", "%.1f".format(waitS))
        NetDiag.log("[rebuffer] 起播预置 cache-pause-wait=${"%.1f".format(waitS)}s 写入=$ok")
        if (!ok) NetDiag.log("[rebuffer] ⚠ wait 运行时写不进 ⇒ 需改 C++ init 阶段")
    }

    /**
     * 由 [MpvCacheDiag.sample] 每 2 s 喂一次。
     *
     * **绝不绑首帧**（方案 B 的教训）。initial buffering 只有攒够 [waitS] 才会
     * 自然结束（playloop: `duration < cache_pause_wait` 才保持暂停），所以
     * runway ≥ [waitS] 就是"预缓冲已完成"的可靠信号；超时兜底防慢源站卡死。
     * 只在状态翻转时下发一次，避免每 2 s 抖动内核属性。
     */
    fun note(native: MpvSession, runwaySec: Double, underrun: Boolean) {
        if (!enabled || restored) return
        val timedOut = System.currentTimeMillis() - armedAtMs > ARM_TIMEOUT_MS
        val reached = runwaySec >= waitS
        if (reached || timedOut) {
            restore(native, if (timedOut) "超时兜底(${ARM_TIMEOUT_MS / 1000}s)" else "runway达标 ${"%.1f".format(runwaySec)}s")
        }
    }

    private fun restore(native: MpvSession, why: String) {
        synchronized(lock) {
            if (restored) return
            restored = true
        }
        val okI = if (useInitial) native.setProperty("cache-pause-initial", "no") else true
        val okW = native.setProperty("cache-pause-wait", "%.1f".format(RELAXED_WAIT_S))
        NetDiag.log(
            "[rebuffer] 恢复默认（$why）：initial=no(写入=$okI) " +
                "wait=${"%.1f".format(RELAXED_WAIT_S)}s(写入=$okW)",
        )
    }

    private fun prop(name: String): String? = try {
        Class.forName("android.os.SystemProperties")
            .getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }
}
