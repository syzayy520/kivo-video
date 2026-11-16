package com.kivo.cinema.playback

import android.content.Context
import android.os.SystemClock
import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.net.NetworkLinkMonitor
import org.json.JSONObject
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.TimeUnit

/**
 * 273 v19：**demuxer 缓存归因诊断** —— 定期读 mpv 的缓存状态并落盘。
 *
 * ## 为什么需要它
 *
 * 播放期实测 `TOTAL PSS` = 584.6 MiB，超出 < 500 MB 的目标（273-D 的判据）。
 * Native Heap 占 336 MB，但**谁占的没有证据**：可能是 mpv 的 demux 缓存
 * （前向 `demuxer-max-bytes` 64 MiB + 回看 16 MiB）、我们的取数环（每读者 8 MiB）、
 * MediaCodec 的解码缓冲、或者预览内核那**第二套** mpv。
 *
 * **没有归因就没有优化** —— 拍脑袋调参数是在赌，赌输了还欠了一笔技术债。
 *
 * ## 读的是什么
 *
 * mpv 的 `demuxer-cache-state`（节点属性，mpv 0.39）：
 * · `fw-bytes`      前向重读队列的字节数（**还会**往源站要多少）
 * · `total-bytes`   整个包队列的字节数（demux 缓存实际占用）
 * · `cache-duration` 缓存覆盖的媒体时长
 * · `underrun`       取数线程满足不了解码器（**demux 侧**欠载，与 AO 欠载是两回事）
 * · `raw-input-rate` 估算的输入速率
 *
 * ## 为什么是永久件
 *
 * 内存与"超买"两个缺口都要靠它**反复复算**；一次性探针收口时会被删掉，
 * 证据就断了 —— 而断掉的证据等于没测过。
 */
object MpvCacheDiag {

    /** 连续读不到多少次就摘除（会话已释放时属性读不到）。 */
    private const val MAX_MISSES = 3

    private const val PERIOD_MS = 2_000L

    @Volatile
    private var session: MpvSession? = null

    private var worker: ScheduledExecutorService? = null

    private var misses = 0

    /** 是否已经读到过一次（读不到 ≠ 会话没了 —— 媒体还没加载时属性根本不存在）。 */
    private var seenOnce = false

    /** 查网络可达性用（`NetworkLinkMonitor` 是静态查询，见其文件头）。 */
    private var appContext: Context? = null

    /**
     * 挂上会话，开始采样（幂等；换会话会重新开始计）。
     *
     * 只给**主播放**内核挂 —— 预览内核不挂，否则两套缓存的数字混在一起。
     *
     * @param context 用来查网络可达性（301 的 `network_recovery_ms`）；null 则跳过网络判定
     */
    @Synchronized
    fun attach(target: MpvSession, context: Context? = null) {
        if (session === target && worker != null) return
        stop()
        session = target
        appContext = context?.applicationContext
        misses = 0
        seenOnce = false
        // 301：新会话 ⇒ 网络恢复的状态清零，不跨会话残留。
        NetworkRecoveryWatch.reset()
        val executor = Executors.newSingleThreadScheduledExecutor { r ->
            Thread(r, "KivoCacheDiag").apply { isDaemon = true }
        }
        worker = executor
        executor.scheduleWithFixedDelay({ sample() }, 0, PERIOD_MS, TimeUnit.MILLISECONDS)
    }

    /** 摘掉会话并停采样（幂等）。 */
    @Synchronized
    fun stop() {
        worker?.shutdownNow()
        worker = null
        session = null
        // 291：会话结束，闸门的待办与压力态也归零，不跨会话残留。
        OriginSupplyPressure.reset()
        // 292 · 注意：**不要**在这里 reset [RebufferPolicy]。
        // attach() 开头会先调 stop()，而 beforeLoad() 发生在它之前
        // （applyPlaybackPolicy → loadFile → attach）。若在此清零 armedAtMs，
        // note() 会恒判「超时兜底」并在首帧同一毫秒写回默认 ⇒ 方案 C 退化成方案 B
        // （292 第三轮真机取证）。rebuffer 的 armed/restored 由每次 beforeLoad() 重设，
        // 本就不需要跨会话清理。
    }

    private fun sample() {
        val target = session ?: return stop()
        // 301：网络恢复计时**独立于缓存采样** —— 断网时 demuxer-cache-state 仍可读，
        // 但位置已经停了，正是要抓的时刻。放在前面，避免被下面的 early return 跳过。
        feedRecoveryWatch(target)
        val raw = target.property("demuxer-cache-state")
        if (raw == null) {
            // 媒体还没加载时属性不存在 —— 那不是"会话没了"，继续等。
            // 只有**读到过**之后再连续读不到，才说明会话被释放了。
            if (!seenOnce) return
            if (++misses >= MAX_MISSES) stop()
            return
        }
        seenOnce = true
        misses = 0
        val node = runCatching { JSONObject(raw) }.getOrNull() ?: return
        val runwaySec = node.optDouble("cache-duration")
        val underrun = node.optBoolean("underrun")
        // 291：runway 是唯一能反映「主链路还够不够播」的信号
        // （275 §6.1 buffer_runway_ms），喂给供数压力闸门决定低优先级预取让不让路。
        OriginSupplyPressure.note(runwaySec, underrun)
        // 292 · 方案 C：起播预缓冲是否攒够，复用同一份 runway 判定
        // （达标或超时才把 cache-pause-initial/wait 写回默认，绝不绑首帧）。
        RebufferPolicy.note(target, runwaySec, underrun)
        NetDiag.log(
            "[cache] fw=${mib(node.optLong("fw-bytes"))} " +
                "total=${mib(node.optLong("total-bytes"))} " +
                "dur=${"%.1f".format(runwaySec)}s " +
                "underrun=$underrun " +
                "rate=${mib(node.optLong("raw-input-rate"))}/s"
        )
    }

    /**
     * 301：把这一拍的"位置 / 是否暂停 / 是否在线"喂给 [NetworkRecoveryWatch]。
     *
     * 每 2 s 一拍 ⇒ 恢复耗时的分辨率约 ±2 s（比 P253 手工测的 2.0–2.2 s 粗，
     * 但那是人工操作，这个是常驻自动采集）。
     */
    private fun feedRecoveryWatch(target: MpvSession) {
        val posSec = runCatching { target.positionSeconds() }.getOrDefault(-1.0)
        val paused = runCatching { target.property("pause") }.getOrNull()
            ?.trim()?.lowercase()?.let { it == "yes" || it == "true" } ?: false
        val online = appContext?.let { NetworkLinkMonitor.isOnline(it) } ?: true
        NetworkRecoveryWatch.note(posSec, paused, online, SystemClock.elapsedRealtime())
    }

    private fun mib(bytes: Long): String = "%.1fM".format(bytes / 1048576.0)
}
