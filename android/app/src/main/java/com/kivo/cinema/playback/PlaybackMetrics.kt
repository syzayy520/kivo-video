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
 * | `seek_total_ms` | 暂未埋点 | ❌ not_measured |
 * | `network_recovery_ms` | 暂未埋点 | ❌ not_measured |
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
    private var label: String = ""
    private var renderMode: String = ""

    /**
     * 开始一轮记录。**每个起播入口调一次**。
     *
     * @param label 取证标签（`kivo.label`），决定输出文件名。
     * @param renderMode 出画通路（写进 JSON，便于按通路分组对比）。
     */
    fun begin(label: String, renderMode: String) {
        beginMs.set(System.currentTimeMillis())
        marks.clear()
        notes.clear()
        this.label = label.ifBlank { "play" }
        this.renderMode = renderMode
    }

    /** 打一个阶段点（同名重复调用保留**第一次**，避免被后来的覆盖）。 */
    fun mark(stage: String) {
        val t = beginMs.get()
        if (t == 0L) return
        marks.putIfAbsent(stage, System.currentTimeMillis())
    }

    /** 记一条自由说明（进 JSON 的 notes）。 */
    fun note(key: String, value: String) {
        notes[key] = value
    }

    /** 是否已经开始记录（没 begin 就不写文件，避免产出垃圾 JSON）。 */
    fun isActive(): Boolean = beginMs.get() != 0L

    /**
     * 落盘成 JSON。
     *
     * @param native 活着的内核（可为 null：起播失败时也要落，记录失败在哪一步）。
     * @param failure 失败原因（成功传 null）。
     */
    fun finish(context: Context, native: MpvSession?, failure: String? = null) {
        val t0 = beginMs.get()
        if (t0 == 0L) return
        val now = System.currentTimeMillis()

        fun since(stage: String): Long? = marks[stage]?.let { it - t0 }
        fun between(from: String, to: String): Long? {
            val a = marks[from] ?: return null
            val b = marks[to] ?: return null
            return b - a
        }

        // mpv 属性：读不到就是 not_measured（**不是 0**）
        fun prop(name: String): String? =
            runCatching { native?.property(name) }.getOrNull()?.takeIf { it.isNotBlank() }

        val runway = prop("demuxer-cache-time")?.let { "${(it.toDoubleOrNull() ?: return@let null)?.times(1000)?.toLong()}" }
            ?: "not_measured: mpv 属性 demuxer-cache-time 读不到"
        val dropVo = prop("vo-drop-frame-count") ?: "not_measured: mpv 属性读不到"
        val dropTotal = prop("frame-drop-count") ?: "not_measured: mpv 属性读不到"
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
        sb.append("    \"frames_dropped_total\": ").append(if (dropTotal.startsWith("not_measured")) q(dropTotal) else dropTotal).append(",\n")
        // render_gap 由设备侧量具采（SurfaceFlinger latency），这里不重复也不假装
        sb.append("    \"render_gap_ms\": ").append(q("not_measured: 由 tools/p274_sf_latency.mjs 采 SurfaceFlinger latency")).append(",\n")
        sb.append("    \"read_latency_ms\": ").append(q("not_measured: 见 OriginLatencyStats（取数层自带统计）")).append(",\n")
        sb.append("    \"seek_total_ms\": ").append(q("not_measured: 尚未埋点")).append(",\n")
        sb.append("    \"network_recovery_ms\": ").append(q("not_measured: 尚未埋点")).append("\n")
        sb.append("  },\n")

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
        sb.append("  \"wall_ms\": {")
        sb.append("\"begin\": ").append(t0).append(", \"finish\": ").append(now)
        sb.append("}\n")
        sb.append("}\n")

        runCatching {
            val dir = File(context.applicationContext.filesDir, "metrics").apply { mkdirs() }
            // 文件名带时刻，避免同一 label 反复起播互相覆盖
            val f = File(dir, "${label}-$now.json")
            f.writeText(sb.toString())
        }
        beginMs.set(0L)
    }

    private fun num(v: Long?): String = v?.toString() ?: q("not_measured")

    /** 读 Android 系统属性（隐藏 API，用反射；拿不到返回 null，**不伪造**）。 */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        (m.invoke(null, name, "") as? String)?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }

    /** JSON 字符串转义（只处理必需字符，标签/属性值都是 ASCII 或中文）。 */
    private fun q(s: String): String {
        val sb = StringBuilder(s.length + 2)
        sb.append('"')
        for (c in s) {
            when (c) {
                '"' -> sb.append("\\\"")
                '\\' -> sb.append("\\\\")
                '\n' -> sb.append("\\n")
                '\r' -> sb.append("\\r")
                '\t' -> sb.append("\\t")
                else -> if (c < ' ') sb.append("\\u").append(String.format("%04x", c.code)) else sb.append(c)
            }
        }
        sb.append('"')
        return sb.toString()
    }
}
