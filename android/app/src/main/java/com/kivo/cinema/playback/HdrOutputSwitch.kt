package com.kivo.cinema.playback

import android.content.Context

/**
 * 278-P2 · **真 HDR 输出（面板收到 PQ）的开关**。
 *
 * ## 为什么单独一个开关，而不是跟着 [PlaybackRenderMode] 走
 *
 * 真 HDR 输出要**四层**配合（10bit Surface / dataspace / EGL PQ colorspace /
 * libplacebo 输出 PQ），其中第 ③ 层要改 mpv 内核。
 * ⇒ 它必须能**独立地**关掉，不能因为切渲染器就自动开 ⇒ 独立开关。
 *
 * ## 分层（与 [PlaybackRenderMode] 同一套惯例）
 *
 * 1. **系统属性** `debug.kivo.hdrOutput` = `off` ⇒ 最高优先，紧急回退。
 * 2. **用户设置项**「视频输出 → HDR 输出」= 自动 / 开 / 关。
 * 3. **默认 = 关**（`auto`）。
 *
 * ## 为什么默认是"关"而不是"自动"
 *
 * 真 HDR 输出的收益只在**少数 HDR 片源 + 支持的设备**上出现；
 * 而代价是"Surface 格式变了"（10bit vs 8bit）⇒ 在不支持的设备上有黑屏风险。
 * ⇒ 宁可默认关，让**开关 + 失败自动退回 8bit** 的组合来保证安全。
 *
 * ## 与 [PlaybackRenderMode] 的关系
 *
 * 真 HDR 输出**只在 gpu-next 通路上有意义**（legacy RA 没有 libplacebo，
 * libplacebo 那边根本不参与 ⇒ 没有"输出 PQ"这回事）。
 * ⇒ legacy 通路上这个开关一律无效（上层直接忽略）。
 */
internal object HdrOutputSwitch {

    /** 不输出真 HDR（默认）。 */
    const val OFF = "off"

    /** 尝试真 HDR 输出（失败自动退回 SDR）。 */
    const val ON = "on"

    /** 自动（当前等价于关；将来可按设备能力放开）。 */
    const val AUTO = "auto"

    /**
     * 是否尝试真 HDR 输出。
     *
     * @param renderMode 当前出画通路（只有 gpu-next 才可能真 HDR）
     */
    fun isEnabled(context: Context, renderMode: String): Boolean {
        // legacy 通路没有 libplacebo ⇒ 真 HDR 无从谈起，直接否。
        if (renderMode != PlaybackRenderMode.GPU_NEXT) return false

        // 设备能力闸门：gralloc 不支持 10bit buffer 时，**必须留在 SDR**。
        // 【为什么这是硬闸门而不是提示】让 mpv 用 10bit EGL config 之后，
        // gralloc 分配 10bit buffer 失败/卡死 ⇒ VO 初始化不返回 ⇒ 整段卡死。
        // 真机 K40 实测：内核日志停在 EGLConfig dump（见 279 差距表 §三）。
        //
        // 【降级留痕在探测者自己那边打】（[Hdr10BitProbe.supports10Bit] 首次探测时打一次）
        // —— 280 之前这句在这里，每调一次 isEnabled 就刷一遍，实测起播刷了 3 次。
        if (!Hdr10BitProbe.supports10Bit()) return false

        // 1) 系统属性（最高优先）
        val prop = sysProp("debug.kivo.hdrOutput")?.lowercase()
        if (prop == OFF || prop == "0" || prop == "false") return false
        if (prop == ON || prop == "1" || prop == "true") return true

        // 2) 用户设置
        val pref = PlaybackPrefs(context).hdrOutput
        if (pref == OFF) return false
        if (pref == ON) return true

        // 3) 默认：关
        return false
    }

    /** 读系统属性（隐藏 API，用反射；拿不到当"没设"）。 */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        val v = m.invoke(null, name, "") as? String
        v?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }
}
