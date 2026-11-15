package com.kivo.cinema.playback

import android.content.Context
import android.view.WindowManager
import com.kivo.cinema.net.NetDiag

/**
 * 帧插值（MEMC）的**参数解析**：决定目标刷新率，并留下取证记录。
 *
 * 真正的 mpv 选项下发在 native 侧（`mpv_options.cpp`），原因是
 * **`display-fps` 只能在 `mpv_initialize` 之前设** —— 真机两条路都试过：
 *
 * ```
 * Set property: display-fps="60.000004"         -> -2   (PROPERTY_NOT_FOUND)
 * Set property: options/display-fps="60.000004" -> -3   (PROPERTY_UNAVAILABLE)
 * ```
 *
 * 所以这里只做两件事：**算**（要几赫兹）与**记账**（日志里能查到这一场开没开）。
 *
 * ## 为什么 `display-fps` 是这一项的生死线
 *
 * 只设 `video-sync=display-resample` + `interpolation=yes` + `tscale=oversample`
 * 三个参数时，真机内核日志是：
 *
 * ```
 * Set property: video-sync="display-resample" -> 1     ← 看起来成功了
 * Set property: interpolation="yes"           -> 1     ← 看起来成功了
 * [vo/gpu-next/libplacebo] Estimated source FPS: 25.000, display FPS: 25.000
 * [vo/gpu-next/libplacebo] Detected fps ratio 0.0000 below threshold 0.0100, disabling interpolation
 * ```
 *
 * gpu-next 在 Android 上走 `wid` 路径时**拿不到显示刷新率**，就用视频帧率顶替
 * ⇒ "源 25 / 显示 25" ⇒ 它自己判定"不需要插值"并关掉。
 * 实测上屏帧率 25.00 → 24.81 fps（不但没涨，p99 还从 41.4 涨到 50.6 ms）——
 * 这就是"参数全 ok、功能是假的"。
 *
 * ## 与 SenPlayer 的关系
 *
 * 它播 24 fps 片源上屏 34.19 fps，但**同时有 3 次 >200 ms 停顿、最长 3.22 s**。
 * 说明它知道自己该输出 60 fps（说明它正确拿到了刷新率），只是 GPU 只做到 34。
 * 我们把刷新率喂对之后，能到多少取决于 K40 的 Adreno 650 —— 必须实测，不许假设。
 */
internal object VideoInterpolation {

    /**
     * 一次插值决策的产物。
     *
     * @param enabled 是否启用插值（最终由 [VideoInterpolationSwitch] 决定）。
     * @param displayFps 设备当前显示刷新率（Hz）；<= 0 = 读不到，插值必然失效。
     */
    class Plan(
        val enabled: Boolean,
        val displayFps: Double,
    ) {
        /** 是否真的能生效（开了 + 拿到了刷新率）。 */
        val effective: Boolean get() = enabled && displayFps > 0.0
    }

    /**
     * 解析插值参数并写诊断日志。
     *
     * @param context 用于读设备刷新率。
     * @param enabled 是否启用（来自 [VideoInterpolationSwitch]）。
     */
    fun plan(context: Context, enabled: Boolean): Plan {
        if (!enabled) {
            NetDiag.log("[interp] 帧插值=关 ⇒ video-sync=audio（默认，与旧内核一致）")
            return Plan(false, 0.0)
        }
        val hz = refreshRateHz(context)
        if (hz > 0.0) {
            NetDiag.log(
                "[interp] 帧插值=开 ⇒ video-sync=display-resample interpolation=yes " +
                    "tscale=oversample display-fps=${"%.3f".format(hz)}（设备 ${"%.1f".format(hz)}Hz）"
            )
        } else {
            // 不许静默：读不到刷新率就说清后果（插值会被内核自己关掉）。
            NetDiag.log("[interp] 帧插值=开但读不到刷新率 ⇒ display-fps 不设 ⇒ 内核会自己禁用插值")
        }
        return Plan(true, hz)
    }

    /**
     * 设备当前显示刷新率（Hz）；读不到返回 0。
     *
     * 【为什么用 `mode.refreshRate` 而不是 `display.refreshRate`】
     * K40 支持 60/90/120 三档自适应，`mode` 是**当前生效的那一档**
     * （真机 `mActiveModeId=1` ⇒ 60.000004），而 `refreshRate` 在某些 ROM 上
     * 返回"首选模式"而不是当前模式。
     *
     * 【为什么读不到不兜底成 60】兜底会让"这台设备到底几赫兹"变成一个猜测，
     * 而猜错的表现是"插值比例不对但看起来像在工作" —— 比明确失效更难查。
     * 宁可不设，让内核日志把它暴露出来。
     */
    private fun refreshRateHz(context: Context): Double = runCatching {
        val wm = context.getSystemService(WindowManager::class.java)
        wm?.defaultDisplay?.mode?.refreshRate?.toDouble()
    }.getOrNull()?.takeIf { it in 20.0..240.0 } ?: 0.0
}
