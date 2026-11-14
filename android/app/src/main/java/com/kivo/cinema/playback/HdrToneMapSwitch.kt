package com.kivo.cinema.playback

import android.content.Context

/**
 * 278 · **HDR 色调映射（L2）的开关**。
 *
 * ## 为什么要有这个开关
 *
 * L2（`MpvHdrToneMap`）在 HDR 片源上把画面映射回 SDR。
 * 278 要在 **gpu-next**（完整 libplacebo 渲染器 + HDR 峰值计算已开）下重新量一次
 * "到底有没有 L2 差多少" ⇒ 必须有一个**能真正关掉 L2** 的办法。
 *
 * 【为什么不能靠 mpv 参数关】L2 是我们在 `MpvStartSequence` 里**主动下发**的
 * 5 个 mpv 选项（`target-trc` / `target-prim` / `target-peak` / `tone-mapping` /
 * `target-colorspace-hint`）。要"没有 L2"，就得**根本不下发它们** ⇒ 只能在这里拦。
 *
 * 【为什么不做成设置项】它不是用户该管的东西（用户要的是"对"），
 * 而是**排障/取证**用的 ⇒ 放系统属性（与 `debug.kivo.renderMode` 同一套惯例）。
 *
 * ## 用法
 *
 * ```bash
 * adb shell setprop debug.kivo.hdrToneMap off   # 关（量真基线用）
 * adb shell setprop debug.kivo.hdrToneMap on    # 开（默认）
 * adb shell setprop debug.kivo.hdrToneMap ''     # 清除 ⇒ 回到默认（开）
 * ```
 */
internal object HdrToneMapSwitch {

    /**
     * L2 是否启用。
     *
     * @param context 用于读设置（预留：将来若要加设置项）
     */
    fun isEnabled(context: Context): Boolean {
        val prop = sysProp("debug.kivo.hdrToneMap")?.lowercase()
        return when (prop) {
            "off", "0", "false", "no" -> false
            else -> true
        }
    }

    /**
     * 读系统属性（隐藏 API，用反射）。
     *
     * 【为什么不用 android.os.SystemProperties 直接调】它是隐藏 API，import 会编译失败。
     * 反射被拦就当"没设" ⇒ 默认值不受影响。
     */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        val v = m.invoke(null, name, "") as? String
        v?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }
}
