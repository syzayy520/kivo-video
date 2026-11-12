package com.kivo.cinema.playback

/**
 * 253-P3：播放内核选择器。
 *
 * ## 为什么需要它
 * 两条内核链路并存：
 *  · **旧内核**（自研 FFmpeg demux + MediaCodec + AAudio）—— 已验证可用，但软解时钟
 *    会乱跳、两套 demux 争用同一条读路径。
 *  · **libmpv 内核**（253）—— 单一 demux 线程 + 自己的时钟 + 全 FFmpeg 解码覆盖，
 *    单一读路径。
 *
 * 切换必须**可回退**：新内核在真实片源上还有长尾问题（DRM、字幕、弹幕、
 * 特殊音轨），不能一次切换就断掉用户能用的路径。
 *
 * ## 怎么选
 * 优先看系统属性（`adb shell setprop` 就能切，无需改 App）：
 *  · `debug.kivo.engine` = `mpv` → 强制走新内核
 *  · `debug.kivo.engine` = `legacy` → 强制走旧内核
 *  · 未设置 → 由 [defaultEngine] 决定（当前默认新内核，因为它在最难片源上更稳）
 *
 * 【为什么用系统属性而不是设置项 UI】
 * 这一阶段的目标是**验证内核本身**，不是做产品化的引擎切换 UI。
 * 属性开关让"在真机上对比两条链路"变成一条命令，不用每次改代码重打包。
 * 产品化时再把它接到设置页（那时它应该是一个持久化的用户偏好，而不是 debug 属性）。
 */
object PlaybackEngine {

    /** 内核标识。 */
    enum class Engine { LEGACY, MPV }

    /** 系统属性名：值为 `mpv` / `legacy`。 */
    const val PROPERTY = "debug.kivo.engine"

    /** 默认内核。新内核在最难片源（HEVC 10bit + TrueHD 5.1）上更稳。 */
    val defaultEngine: Engine = Engine.MPV

    /**
     * 当前应使用的内核。
     *
     * @param propertyReader 读取系统属性；默认走 Android 的 `System.getProperty`。
     *   抽成参数是为了能在 JVM 单元测试里验证选择逻辑（不依赖 Android 运行时）。
     */
    fun select(propertyReader: (String) -> String? = { readSystemProperty(it) }): Engine {
        return when (propertyReader(PROPERTY)?.trim()?.lowercase()) {
            "mpv", "new", "libmpv" -> Engine.MPV
            "legacy", "old", "self" -> Engine.LEGACY
            else -> defaultEngine
        }
    }

    /** 读 Android 系统属性。 */
    private fun readSystemProperty(name: String): String? {
        return try {
            val clazz = Class.forName("android.os.SystemProperties")
            val get = clazz.getMethod("get", String::class.java)
            get.invoke(null, name) as? String
        } catch (e: Exception) {
            // 非 Android 环境（JVM 测试）读不到 —— 返回 null 走默认。
            null
        }
    }
}
