package com.kivo.cinema.playback

import com.kivo.cinema.net.NetDiag
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit

/**
 * 291：**供数压力闸门** —— 把 275 §5.2 的优先级契约落成可执行策略。
 *
 * ## 为什么要它（真机 p290-live）
 *
 * 275 §5.2 写明：起播/seek/播放中的读取优先级是
 * 「当前解码所需字节 > seek 落点窗口 > 短时前瞻 > **低优先级预取**」，
 * 且「并发预读只在同源真机数据证明源站允许叠加带宽后启用」。
 *
 * 但现有实现里低优先级预取是**无条件**跑的：
 *  · [com.kivo.cinema.net.OriginWarmupPool] 每 8 s 一轮保温 ping；
 *  · `startUrlWarmup()` 在首帧后立刻起保温池 + cues 索引（旧注释断言
 *    「首帧之后带宽空闲」，被慢源站实测推翻）；
 *  · `prewarmThumbnails()` 起**第二个 mpv 内核**读片源。
 *
 * 夜间公网源吞吐 417 KB/s < 片源需求 501 KB/s 时，127.8 s 冻结窗口内
 * 主 chunk 只有 1 条，warmup ping 却有 52 次（每次都要借/建一条源站连接，
 * 表头 1–3 s）。源站连接额度被低优先级动作持续占用，主链路恢复更慢。
 * A 臂那次起播 buffering（1.95 s）也落在首帧后 warmup/cue/thumb 抢连接的窗口。
 *
 * ## 做法
 *
 * [MpvCacheDiag] 每 2 s 把 `cache-duration`（275 §6.1 的 `buffer_runway_ms`）
 * 与 `underrun` 喂进 [note]。闸门用**滞回**判定压力：
 *  · runway < [LOW_RUNWAY_S] 或 `underrun` ⇒ 立刻进入压力态；
 *  · runway ≥ [HIGH_RUNWAY_S] 连续 [RELIEVE_SAMPLES] 次 ⇒ 才解除。
 * 中间带保持不变，避免阈值边缘反复抖动。
 *
 * 压力态下：保温 ping 停发（[shouldDeferLowPriority]），
 * 一次性预热（[defer]）延后到压力解除再放行。
 * **不是丢弃**：源站持续不足时用 [MAX_DEFER_MS] 兜底放行，
 * 保证拖动预览等功能不会永久不可用，且留一行日志说明原因。
 * 没有 cache 采样时（[STALE_MS]）视为「无压力判据」，同样放行 ——
 * 不能因为诊断没挂就把功能锁死。
 *
 * ## 开关
 *
 * `adb shell setprop debug.kivo.supplyPressure off` ⇒ 完全关闭（A/B 对照臂）：
 * [shouldDeferLowPriority] 恒 false、[defer] 立即执行、[note] 直接返回，
 * 既有取数行为一字不变。
 */
object OriginSupplyPressure {

    private const val PROPERTY = "debug.kivo.supplyPressure"

    /** 低于这个前向 runway（秒）判供数压力。 */
    private const val LOW_RUNWAY_S = 8.0

    /** 高于这个 runway 且连续健康才解除（滞回，避免抖动）。 */
    private const val HIGH_RUNWAY_S = 16.0

    /** 解除压力所需的连续健康采样数（每次 2 s）。 */
    private const val RELIEVE_SAMPLES = 3

    private const val TICK_MS = 2_000L

    /** 超过这么久没有 cache 采样 ⇒ 不作为压力判据。 */
    private const val STALE_MS = 8_000L

    /** 压力持续不解除时的兜底放行上限。 */
    private const val MAX_DEFER_MS = 45_000L

    private val lock = Object()

    private class Deferred(val atMs: Long, val action: () -> Unit)

    private val pending = mutableListOf<Deferred>()

    private val enabled: Boolean = readSystemProperty(PROPERTY)?.trim()?.lowercase() != "off"

    /** 起播默认保守：还没采到健康 runway 之前，不让低优先级预取抢连接。 */
    @Volatile private var pressured = true

    @Volatile private var lastNoteMs = 0L

    /** 第一次 [defer] 的时刻；从未收到采样时用它做 stale 基准。 */
    @Volatile private var armMs = 0L

    private var healthySamples = 0

    private var worker: ScheduledExecutorService? = null

    init {
        if (!enabled) NetDiag.log("[supply] 压力闸门已关闭（A/B 对照臂）")
    }

    /**
     * 喂一次 `demuxer-cache-state` 采样（由 [MpvCacheDiag] 调用，每 2 s）。
     *
     * @param runwaySec 前向媒体可播时长（275 §6.1 `buffer_runway_ms` / 1000）
     * @param underrun  demux 侧是否已欠载
     */
    fun note(runwaySec: Double, underrun: Boolean) {
        if (!enabled) return
        lastNoteMs = System.currentTimeMillis()
        val low = underrun || runwaySec < LOW_RUNWAY_S
        val high = !underrun && runwaySec >= HIGH_RUNWAY_S
        synchronized(lock) {
            when {
                low -> {
                    healthySamples = 0
                    if (!pressured) {
                        pressured = true
                        NetDiag.log(
                            "[supply] 供数压力 ON：runway=${"%.1f".format(runwaySec)}s underrun=$underrun",
                        )
                    }
                }
                high -> {
                    healthySamples++
                    if (pressured && healthySamples >= RELIEVE_SAMPLES) {
                        pressured = false
                        NetDiag.log(
                            "[supply] 供数压力 OFF：runway=${"%.1f".format(runwaySec)}s " +
                                "连续 $healthySamples 次健康",
                        )
                    }
                }
                else -> healthySamples = 0 // 滞回带内保持当前状态
            }
        }
    }

    /** 低优先级预取（保温 ping）每轮查询：true = 这一轮别去抢源站连接。 */
    fun shouldDeferLowPriority(): Boolean = enabled && pressured

    /**
     * 首帧后的一次性预热：压力期延后，解除后（或兜底超时）放行一次。
     *
     * 关闭开关或当前无压力时**立即执行**，保证对照臂与旧行为完全一致。
     */
    fun defer(action: () -> Unit) {
        if (!enabled || !pressured) {
            action()
            return
        }
        val now = System.currentTimeMillis()
        if (armMs == 0L) armMs = now
        synchronized(lock) { pending += Deferred(now, action) }
        ensureWorker()
    }

    /** 会话切换时清空待办（不跨会话残留），压力态回到起播默认值。 */
    fun reset() = synchronized(lock) {
        pending.clear()
        healthySamples = 0
        lastNoteMs = 0L
        armMs = 0L
        pressured = true
    }

    private fun tick() {
        val now = System.currentTimeMillis()

        // 没有诊断数据时不能把低优先级功能永久锁死（例如 legacy 会话未挂 diag）。
        // 从未收到采样时以第一次 defer 的时刻为基准，不能因 lastNoteMs==0 永不解除。
        val baseline = if (lastNoteMs > 0L) lastNoteMs else armMs
        if (enabled && baseline > 0L && now - baseline > STALE_MS && pressured) {
            synchronized(lock) {
                pressured = false
                healthySamples = 0
            }
            NetDiag.log("[supply] 压力解除：${STALE_MS / 1000}s 无 cache 采样，不作为压力判据")
        }

        val due = mutableListOf<() -> Unit>()
        var timedOut = 0
        synchronized(lock) {
            val it = pending.iterator()
            while (it.hasNext()) {
                val d = it.next()
                when {
                    !pressured -> {
                        due += d.action
                        it.remove()
                    }
                    now - d.atMs >= MAX_DEFER_MS -> {
                        due += d.action
                        timedOut++
                        it.remove()
                    }
                }
            }
        }
        if (due.isNotEmpty()) {
            NetDiag.log("[supply] 放行 ${due.size} 项延后预热（超时兜底 $timedOut 项）")
        }
        // 这些动作只是起线程 / postDelayed，放在本线程执行不会阻塞呈现线程。
        due.forEach {
            runCatching { it.invoke() }.onFailure { e ->
                NetDiag.log("[supply] 预热动作失败：${e.message}")
            }
        }
    }

    @Synchronized
    private fun ensureWorker() {
        if (worker != null || !enabled) return
        worker = Executors.newSingleThreadScheduledExecutor { r ->
            Thread(r, "KivoSupplyPressure").apply { isDaemon = true }
        }.also {
            it.scheduleWithFixedDelay(
                { runCatching { tick() } },
                TICK_MS,
                TICK_MS,
                TimeUnit.MILLISECONDS,
            )
        }
    }

    private fun readSystemProperty(name: String): String? = try {
        val clazz = Class.forName("android.os.SystemProperties")
        clazz.getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }
}
