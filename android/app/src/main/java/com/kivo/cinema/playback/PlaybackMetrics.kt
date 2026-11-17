package com.kivo.cinema.playback

import android.content.Context
import android.os.Build
import com.kivo.cinema.mpv.MpvSession
import java.io.File
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong

/**
 * 280 · 一次播放的**结构化指标**（对齐 275 §6.1 的十个可复算量）。
 *
 * ## 为什么需要它
 *
 * 275 §6.1 的原话是：「把'不卡'变成可复算指标」。我们现在只有自由文本日志
 * （`NetDiag.log("MPV 首帧就绪")`）—— 能看，但**不能汇总、不能跨版本对比、
 * 不能证明"比 SenPlayer 好"**。这个文件把阶段量落成 JSON，供脚本批量对比。
 *
 * ## 铁律：测不到就写 not_measured，**不许写 0**
 *
 * 275 §6.1 明确要求「没有采样的字段写 `not_measured + reason`；不能用 0 表示未知」。
 * 本文件所有字段都遵守这条 —— 读不到 mpv 属性时值是
 * `"not_measured: <原因>"` 字符串，而不是 0。
 *
 * ## 能测 / 不能测（诚实的边界）
 *
 * | 275 的字段 | 我们怎么测 | 状态 |
 * | --- | --- | --- |
 * | `startup_total_ms` | 本文件打点：begin → 首帧回调 | ✅ |
 * | `open_ms` | `loadfile` → 首帧（**含解码**，比 275 的口径宽） | ⚠️ 口径不同，见字段名 |
 * | `first_decode_ms` | mpv 没有对外事件，**测不到** | ❌ not_measured |
 * | `first_present_ms` | 同上（首帧回调已经是"已上屏"） | ❌ not_measured |
 * | `read_latency_ms` | 取数层有自己的统计（`OriginLatencyStats`），不重复采 | ❌ 指向来源 |
 * | `buffer_runway_ms` | mpv 属性 `demuxer-cache-time` | ✅ |
 * | `render_gap_ms` | **SurfaceFlinger latency 量具**（tools/p274_sf_latency.mjs） | ❌ 设备侧采，JSON 指向它 |
 * | `frames_dropped` | mpv 属性 `frame-drop-count` / `vo-drop-frame-count` | ✅ |
 * | `seek_total_ms` | 见 `seeks` 明细（p50/p95/max）；顶层字段仍留 not_measured | ⚠️ 看 seeks |
 * | `network_recovery_ms` | `MpvCacheDiag` 每 2 s 喂 `NetworkRecoveryWatch`（位置停 + 离线 + 时钟再前进） | ✅ |
 * | `thermal_context` | `ThermalSnapshot`：PowerManager 热状态 + sysfs 最高温 | ✅ |
 *
 * ⇒ **本文件不假装"全都有"**。它把能测的测准，测不到的写清原因，
 * 这样看报告的人一眼就知道哪条结论有数据支撑。
 */
internal object PlaybackMetrics {

    // ---- 打点名（只用常量，避免拼写漂移） ----
    const val T_BEGIN = "begin"
    const val T_SOURCE_READY = "source_ready"
    const val T_KERNEL_CREATED = "kernel_created"
    const val T_LOADFILE = "loadfile"
    const val T_FIRST_FRAME = "first_frame"

    private val beginMs = AtomicLong(0L)
    private val marks = ConcurrentHashMap<String, Long>()
    private val notes = ConcurrentHashMap<String, String>()

    /** 一次 seek 的实测（275 §6.2 要求「连续 20 次 seek 无旧请求覆盖新目标」）。 */
    internal data class SeekRec(
        val requestedUs: Long,
        val landingUs: Long,
        val totalMs: Long,
        val error: String?,
    )
    private val seeks = java.util.Collections.synchronizedList(mutableListOf<SeekRec>())
    private var label: String = ""
    private var renderMode: String = ""
    private val writes = AtomicLong(0L)

    /** 保存上下文与最近一次读到的内核句柄 —— seek 之后要靠它们独立落盘。 */
    @Volatile private var appContext: Context? = null
    @Volatile private var lastNative: MpvSession? = null
    private val mainHandler by lazy { android.os.Handler(android.os.Looper.getMainLooper()) }
    @Volatile private var flushScheduled = false

    /**
     * 开始一轮记录。**每个起播入口调一次**。
     *
     * @param label 取证标签（`kivo.label`），决定输出文件名。
     * @param renderMode 出画通路（写进 JSON，便于按通路分组对比）。
     */
    fun begin(context: Context?, label: String, renderMode: String) {
        appContext = context?.applicationContext
        beginMs.set(System.currentTimeMillis())
        marks.clear()
        notes.clear()
        seeks.clear()
        // 345：快照归 [MpvHealthProbe] 持有，新一场播放清空避免串台。
        MpvHealthProbe.reset()
        writes.set(0L)
        this.label = label.ifBlank { "play" }
        this.renderMode = renderMode
        // 345 · soak 时间序列：与单文件指标同场开、生命周期一致。
        MpvHealthSoak.begin(appContext, this.label)
    }

    /** 打一个阶段点（同名重复调用保留**第一次**，避免被后来的覆盖）。 */
    fun mark(stage: String) {
        val t = beginMs.get()
        if (t == 0L) return
        marks.putIfAbsent(stage, System.currentTimeMillis())
    }

    /**
     * 记一次 seek（成功与失败都记）。
     *
     * @param requestedUs 用户/脚本请求的目标位置。
     * @param landingUs 实际落点（-1 = 没落成）。
     * @param totalMs 请求到落点确认的耗时。
     * @param error 失败原因（成功传 null）。
     */
    fun recordSeek(requestedUs: Long, landingUs: Long, totalMs: Long, error: String?) {
        // 取证：确认这条路径真的被调用（没有它就只能靠猜，见 280 的排查记录）
        com.kivo.cinema.net.NetDiag.log(
            "[metrics] recordSeek active=" + (beginMs.get() != 0L) +
                " req=" + requestedUs / 1000 + "ms land=" + landingUs / 1000 +
                "ms t=" + totalMs + "ms " + (error ?: "ok")
        )
        if (beginMs.get() == 0L) return
        seeks.add(SeekRec(requestedUs, landingUs, totalMs, error))
        scheduleFlush()
    }

    /**
     * 1 s 后落盘（debounce：连续 seek 只写一次）。
     *
     * 【为什么不能只靠生命周期】280 踩过：首帧后 2 s 写的那份只有起播指标，
     * 而 `onStop` 里补写那次实测**没有生效**（`writes` 恒为 1）⇒ seek 数据全丢。
     * 与其赌生命周期，不如在数据产生的当下就安排落盘。
     */
    private fun scheduleFlush() {
        if (flushScheduled) return
        flushScheduled = true
        mainHandler.postDelayed({
            flushScheduled = false
            flush()
        }, 1_000)
    }

    /** 把当前快照写成文件（不重置任何状态）。 */
    fun flush() {
        val ctx = appContext ?: return
        finish(ctx, lastNative, null)
    }

    /** 记一条自由说明（进 JSON 的 notes）。 */
    fun note(key: String, value: String) {
        notes[key] = value
    }

    /** 是否已经开始记录（没 begin 就不写文件，避免产出垃圾 JSON）。 */
    fun isActive(): Boolean = beginMs.get() != 0L

    /**
     * 起播即注册内核句柄（否则 `onStop` 传 null 时无法回落，见 [finish] 里的 `eff`）。
     */
    fun attachNative(native: MpvSession) {
        lastNative = native
    }

    /**
     * 落盘成 JSON。
     *
     * @param native 活着的内核（可为 null：起播失败时也要落，记录失败在哪一步）。
     * @param failure 失败原因（成功传 null）。
     */
    fun finish(context: Context, native: MpvSession?, failure: String? = null) {
        val t0 = beginMs.get()
        if (t0 == 0L) return
        if (context.applicationContext != null) appContext = context.applicationContext
        if (native != null) lastNative = native
        // 345 · 参数为空时回落到已注册句柄。
        // 【为什么必须有这条回落】`onStop` 传的是 `mpvNetSession?.nativeOrNull()`，
        // 而本地 mpv 播放的会话不在 `mpvNetSession` 里 ⇒ 参数恒 null ⇒
        // 整份 JSON 的 mpv_health/metrics 全变 not_measured（真机 r1/r2 复现两次）。
        // 这条回落让"退出时补写"与"播放中 debounce 写"读到同一个内核。
        val eff = native ?: lastNative
        val now = System.currentTimeMillis()

        fun since(stage: String): Long? = marks[stage]?.let { it - t0 }
        fun between(from: String, to: String): Long? {
            val a = marks[from] ?: return null
            val b = marks[to] ?: return null
            return b - a
        }

        // mpv 属性：读不到就是 not_measured（**不是 0**）
        fun prop(name: String): String? =
            runCatching { eff?.property(name) }.getOrNull()?.takeIf { it.isNotBlank() }

        val runway = prop("demuxer-cache-time")?.let { "${(it.toDoubleOrNull() ?: return@let null)?.times(1000)?.toLong()}" }
            ?: "not_measured: mpv 属性 demuxer-cache-time 读不到"
        // 【属性名以 mpv 源码为准，不要凭印象写】
        // `player/command.c:4020-4021`：
        //   {"decoder-frame-drop-count", mp_property_frame_drop_dec}  ← 解码层丢帧
        //   {"frame-drop-count",         mp_property_frame_drop_vo }  ← VO 丢帧
        // 之前写的 `vo-drop-frame-count` **根本不存在** ⇒ 一直读不到（JSON 里恒为 not_measured）。
        val dropVo = prop("frame-drop-count") ?: "not_measured: mpv 属性读不到"
        val dropDec = prop("decoder-frame-drop-count") ?: "not_measured: mpv 属性读不到"
        val videoParams = prop("video-params") ?: "not_measured: mpv 属性读不到"

        val sb = StringBuilder()
        sb.append("{\n")
        sb.append("  \"label\": ").append(q(label)).append(",\n")
        sb.append("  \"render_mode\": ").append(q(renderMode)).append(",\n")
        sb.append("  \"device\": {")
        sb.append("\"model\": ").append(q(Build.MODEL))
        // 【不能用 Build.HARDWARE】真机实测它返回的是一串通用值 "android"，
        // 不是 SoC（K40 的真实值是 kona = 高通 SM8250）。`ro.board.platform` 才对。
        sb.append(", \"soc\": ").append(q(sysProp("ro.board.platform") ?: Build.HARDWARE))
        sb.append(", \"android\": ").append(Build.VERSION.SDK_INT)
        sb.append("},\n")

        sb.append("  \"stages_ms\": {\n")
        sb.append("    \"startup_total\": ").append(num(since(T_FIRST_FRAME))).append(",\n")
        sb.append("    \"source_ready\": ").append(num(since(T_SOURCE_READY))).append(",\n")
        sb.append("    \"kernel_created\": ").append(num(since(T_KERNEL_CREATED))).append(",\n")
        sb.append("    \"loadfile_to_first_frame\": ").append(num(between(T_LOADFILE, T_FIRST_FRAME))).append(",\n")
        sb.append("    \"first_decode\": ").append(q("not_measured: mpv 未对外暴露解码完成事件")).append(",\n")
        sb.append("    \"first_present\": ").append(q("not_measured: 首帧回调本身已是\"已上屏\"，无法再分段")).append("\n")
        sb.append("  },\n")

        sb.append("  \"metrics\": {\n")
        sb.append("    \"buffer_runway_ms\": ").append(if (runway.startsWith("not_measured")) q(runway) else runway).append(",\n")
        sb.append("    \"frames_dropped_vo\": ").append(if (dropVo.startsWith("not_measured")) q(dropVo) else dropVo).append(",\n")
        sb.append("    \"frames_dropped_decoder\": ").append(if (dropDec.startsWith("not_measured")) q(dropDec) else dropDec).append(",\n")
        // render_gap 由设备侧量具采（SurfaceFlinger latency），这里不重复也不假装
        sb.append("    \"render_gap_ms\": ").append(q("not_measured: 由 tools/p274_sf_latency.mjs 采 SurfaceFlinger latency")).append(",\n")
        sb.append("    \"read_latency_ms\": ").append(q("not_measured: 见 OriginLatencyStats（取数层自带统计）")).append(",\n")
        // 275 §6.1 的 seek_total_ms：取本次会话所有 seek 的 p50（代表性单值），
        // 明细与 p95/max 见下面的 "seeks" 区。
        // 【曾经这里写死 "尚未埋点"】但 recordSeek() 早已接入（280），字段没跟上
        // ⇒ JSON 里明明有真数据却报"未测"，会误导读报告的人。
        sb.append("    \"seek_total_ms\": ")
        if (seeks.isEmpty()) sb.append(q("not_measured: 本次会话未发生 seek"))
        else sb.append(seeks.map { it.totalMs }.sorted()[seeks.size / 2])
        sb.append(",\n")
        // 301：网络恢复 → 媒体时钟再次前进的耗时。
        // 由 MpvCacheDiag 每 2 s 喂 NetworkRecoveryWatch（位置/暂停/在线三态状态机）。
        val recovery = NetworkRecoveryWatch.lastRecoveryMs()
        sb.append("    \"network_recovery_ms\": ")
        if (recovery >= 0) sb.append(recovery)
        else sb.append(q("not_measured: 本次会话未发生断网→恢复"))
        sb.append(",\n")
        // 301：热状态上下文（275 §6.1 用来解释长时间播放的性能衰减）。
        sb.append("    \"thermal_context\": ").append(ThermalSnapshot.read(appContext)).append("\n")
        sb.append("  },\n")

        sb.append(renderSeekBlock(seeks))

        // 345 · 新宿主自己的可比指标契约（mpv property 实采 + 不可比项声明）。
        // 与上面 metrics 块并列而不合并：那一块是「本次会话的业务指标」，
        // 这一块是「内核健康度」，两者的采集时机与读者都不同。
        sb.append("  \"mpv_health\": ").append(MpvHealthProbe.cachedOrRead(eff)).append(",\n")

        sb.append("  \"video_params\": ").append(q(videoParams)).append(",\n")
        sb.append("  \"failure\": ").append(if (failure == null) "null" else q(failure)).append(",\n")
        sb.append("  \"notes\": {")
        var first = true
        for ((k, v) in notes) {
            if (!first) sb.append(", ")
            first = false
            sb.append(q(k)).append(": ").append(q(v))
        }
        sb.append("},\n")
        sb.append("  \"writes\": ").append(writes.get() + 1).append(",\n")
        sb.append("  \"wall_ms\": {")
        sb.append("\"begin\": ").append(t0).append(", \"finish\": ").append(now)
        sb.append("}\n")
        sb.append("}\n")

        runCatching {
            val dir = File(context.applicationContext.filesDir, "metrics").apply { mkdirs() }
            // 【文件名固定、可反复覆盖写】280 踩过：原来文件名带时刻且写完就把
            // beginMs 清零 ⇒ 首帧后 2 s 写的那一份把指标"封盘"了，
            // 而 seek 压测要到起播后 8 s 才开始 ⇒ `seeks.count` 恒为 0。
            // 现在同一场播放只对应一个文件，后续每次 finish 覆盖它，
            // 文件里用 `writes` 记是第几次写，`wall.finish` 是最后一次写的时刻。
            val f = File(dir, "$label.json")
            f.writeText(sb.toString())
        }
        // 【不清 beginMs】留着，让后续的 seek / 长稳数据还能记进来，
        // 直到下一轮 begin() 或 Activity 销毁时的 finish()。
        writes.incrementAndGet()
    }

    private fun num(v: Long?): String = v?.toString() ?: "\"not_measured\""

    /** 读 Android 系统属性（隐藏 API，用反射；拿不到返回 null，**不伪造**）。 */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        (m.invoke(null, name, "") as? String)?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }

    /** JSON 转义：实现见 [metricsJsonQuote]（拆走后避免本文件继续膨胀）。 */
    private fun q(s: String): String = metricsJsonQuote(s)
}
