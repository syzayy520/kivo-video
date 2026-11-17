package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession

/**
 * 新宿主（mpv）的可比指标契约。
 *
 * ## 为什么需要这个文件
 *
 * 旧宿主 `ui/PlayerActivity` 有 24 个探针、3 份 schema（`kivo.p12c.decode/21`、
 * `kivo.p12c.sync/45`、`kivo.p12f.negative/1`），全部建在自研内核
 * （`DecodeSession` + `FramePacer` + `ClockPositionTracker`）之上。
 * mpv 是自带时钟的，那套探针里有一半没有等价物——照搬只能是「存下来但不生效」
 * 或「伪造同名字段」，两者都是静默降级。
 *
 * 本文件的做法不是「兼容旧字段」，而是**给新宿主立自己的契约**：
 * 1. 只采集 mpv 能真实回答的量，读不到一律 `not_measured: <原因>`，绝不补 0；
 * 2. 用 [INCOMPARABLE] 显式声明「旧指标在本宿主没有等价物」，
 *    防止有人拿 `mistimed-frame-count` 去对旧 `pace.lateness_ns` 做假对比。
 *
 * 契约定义与逐探针分类见 `docs/design/playback/345-mpv-metric-contract.md`。
 */
internal object MpvHealthProbe {

    /**
     * 旧链路有、mpv 没有等价物的指标。
     *
     * 全部指向自研内核的 `ClockPositionTracker::position_ns_` 这一个变量
     * （时钟注入类 6 个探针改的都是它），mpv 没有「往主时钟注入台阶」的接缝。
     * `audio-delay` 只能造恒定 A/V 偏移，造不出「钟比墙快」，因此不可互换。
     */
    private val INCOMPARABLE = listOf(
        "pace.lateness_ns",        // mpv 无 per-frame 迟到量通道
        "pace.schedule_lead_ns",   // 同上，present 时基由 mpv 内部控制
        "pace.drop_ratio",         // 丢帧决策权在 mpv，无「判它太晚」的等价口径
        "clock_injection.*",       // 6 个注入探针无对应能力
        "drift_gate.*",            // mpv 同步策略内建，无「关门」开关与计数
        "counters.frames_dropped_late",
        // 上屏帧率：mpv 无任何 property 等价物（estimated-vf-fps 只是 vf 链估值），
        // 唯一可信来源是 SurfaceFlinger --latency 量具。
        "output_fps（上屏帧率，须 tools/p274_sf_latency 量）",
    )

    /**
     * 输出 `mpv_health` 块（不带外层键名，调用方负责拼 `"mpv_health": `）。
     *
     * @param native 活着的 mpv 会话；null 时全部字段为 `not_measured`（起播失败也要能落盘）。
     */
    fun json(native: MpvSession?): String {
        fun p(name: String): String? =
            if (native == null) null
            else runCatching { native.property(name) }.getOrNull()?.trim()?.takeIf { it.isNotEmpty() }

        val sb = StringBuilder()
        sb.append("{\n")
        // clock_source 是本契约的根标记：报告里看到 mpv，就不该去找 pace 块。
        sb.append("    \"clock_source\": \"mpv\",\n")
        sb.append("    \"video_sync\": ").append(str(p("video-sync"))).append(",\n")

        sb.append("    \"container_fps\": ").append(num(p("container-fps"))).append(",\n")
        // estimated-vf-fps 是 **vf 链估值 ≈ 源帧率**，插值在 VO/display-sync 层做，不经过 vf。
        // 实测（K40，25fps 源 + 插值开，2026-11-18）：本字段 25.0，
        // 而 SurfaceFlinger 上屏 60.54fps、p50 间隔 16.52ms、0 次 >25ms。
        // ⇒ **拿本字段做插值验收会得出"插值没开"的错结论**，上屏帧率只能靠 SF 量具。
        sb.append("    \"estimated_vf_fps\": ").append(num(p("estimated-vf-fps"))).append(",\n")
        // mpv 0.39 的 display-fps 读出来是 -2（PROPERTY_UNAVAILABLE），
        // 真正的刷新率来源是 options/display-fps-override（见 282 帧插值）。
        sb.append("    \"display_fps_override\": ").append(str(p("options/display-fps-override"))).append(",\n")

        // mistimed：display-sync 下本该在 vsync 呈现却没赶上，比 VO 丢帧更能说明卡顿。
        sb.append("    \"mistimed_frame_count\": ").append(num(p("mistimed-frame-count"))).append(",\n")
        sb.append("    \"vo_drop_count\": ").append(num(p("frame-drop-count"))).append(",\n")
        sb.append("    \"decoder_drop_count\": ").append(num(p("decoder-frame-drop-count"))).append(",\n")

        // avsync：最近一次音画差（秒）。纯音频/无音轨时 mpv 不提供，走 not_measured。
        sb.append("    \"avsync_s\": ").append(num(p("avsync"))).append(",\n")
        sb.append("    \"hwdec_current\": ").append(str(p("hwdec-current"))).append(",\n")

        sb.append("    \"incomparable\": [")
        sb.append(INCOMPARABLE.joinToString(", ") { "\"$it\"" })
        sb.append("]\n")
        sb.append("  }")
        return sb.toString()
    }

    /**
     * 播放中采到的最后一份快照。
     *
     * 【为什么要有快照】mpv property 是瞬时值：`onStop` 时 `onPlayerStop()` 已先销毁内核
     * ⇒ 收尾现读全部 `not_measured`（真机 s1/s2/t1/t2 四次复现）。
     */
    @Volatile
    private var cached: String? = null

    /**
     * 采样一次（**必须主线程**调用）。
     *
     * 【线程约束是实测出来的】同一个内核在 `KivoCacheDiag` 后台线程读
     * `estimated-vf-fps` 恒 null，主线程读有值（`demuxer-cache-time` 两边都能读，
     * vo 相关属性只在主线程读到）⇒ 2026-11-18，K40，label u1 vs v1。
     */
    fun sample(native: MpvSession) {
        val next = runCatching { json(native) }.getOrNull() ?: return
        // 媒体未就绪的窗口（实测无音轨片起播后约 6 s 内 `container-fps` 读不到，
        // 此时只有 video-sync / display-fps-override 这类 option 属性可读）：
        // **不覆盖已有有效快照**，否则一次早采样会把它冲成半空 —— 真机 wa 那份
        // 3/10 就是这么来的（同一路径 wb/wc/x1-x3 都是 9/10）。
        if (next.contains("container_fps\": \"not_measured")) {
            if (cached == null) cached = next
            return
        }
        cached = next
    }

    /** 收尾取快照：优先用播放中采到的；没有才现读（内核已销毁则如实全 not_measured）。 */
    fun cachedOrRead(native: MpvSession?): String = cached ?: json(native)

    /** 新一场播放开始清空，避免上一场的快照串台。 */
    fun reset() {
        cached = null
    }

    /** 数值字段：读不到写 `not_measured: mpv 属性读不到`，**不补 0**。 */
    private fun num(v: String?): String =
        if (v == null) "\"not_measured: mpv 属性读不到\"" else v

    /** 枚举/字符串字段：读不到同样回落为带原因的字符串，保持字段恒在。 */
    private fun str(v: String?): String =
        if (v == null) "\"not_measured: mpv 属性读不到\"" else "\"$v\""
}
