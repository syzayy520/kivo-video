package com.kivo.cinema.playback

import android.content.Context
import android.os.Build
import android.os.PowerManager
import java.io.File

/**
 * 280/301 · **热状态采样**（275 §6.1 的 `thermal_context`）。
 *
 * ## 为什么需要
 * 275 §6.1 要求每次播放带 `thermal_context`：「解释长时间性能衰减」。
 * 没有它，"跑了 30 分钟掉帧"这种事永远说不清是代码退化还是机器热了。
 *
 * ## 采什么
 * 1. **系统热状态**（API 29+）：`PowerManager.currentThermalStatus`
 *    —— 这是厂商调过的分级（NONE/LIGHT/MODERATE/SEVERE/CRITICAL/EMERGENCY/SHUTDOWN），
 *    比我们自己读温度阈值更可靠。
 * 2. **最高温区温度**（尽力而为）：扫 `/sys/class/thermal/` 下每个 `thermal_zoneN/temp`。
 *    单位是**毫摄氏度**（43000 = 43 ℃）。SELinux 可能不让读 ⇒ 读不到就写 not_measured。
 *
 * ## 为什么不注册监听
 * `addThermalStatusListener` 要配对注销、要绑生命周期，而我们只需要
 * **一个时间点的上下文**（写进播放结束的 JSON）。一次同步查询足够，
 * 与 [com.kivo.cinema.net.NetworkLinkMonitor] 的取舍同理（静态查询，无状态可泄漏）。
 *
 * ## 铁律
 * 读不到写 `not_measured: <原因>`，**不写 0、不写估算值**。
 */
internal object ThermalSnapshot {

    /** 热状态码 → 可读名（下标即 `PowerManager.THERMAL_STATUS_*` 的值）。 */
    private val STATUS_NAMES = arrayOf(
        "NONE", "LIGHT", "MODERATE", "SEVERE", "CRITICAL", "EMERGENCY", "SHUTDOWN",
    )

    /**
     * 采一次快照，返回可直接嵌进 JSON 的**值片段**（数字 / 字符串 / null）。
     *
     * @return 形如 `{"status":"MODERATE","code":2,"max_temp_c":43.5}`，
     *         或 `"not_measured: ..."`（带引号的字符串，调用方直接 append）
     */
    fun read(context: Context?): String {
        val statusCode = runCatching {
            if (Build.VERSION.SDK_INT < 29 || context == null) return@runCatching null
            val pm = context.getSystemService(PowerManager::class.java)
            pm?.currentThermalStatus
        }.getOrNull()

        val tempC = maxThermalZoneTempC()

        if (statusCode == null && tempC == null) {
            val why = if (Build.VERSION.SDK_INT < 29) {
                "API ${Build.VERSION.SDK_INT} < 29，PowerManager 没有热状态；sysfs 也读不到"
            } else {
                "PowerManager 与 sysfs 都拿不到"
            }
            return "\"not_measured: $why\""
        }

        val sb = StringBuilder("{")
        if (statusCode != null) {
            val name = STATUS_NAMES.getOrElse(statusCode) { "UNKNOWN($statusCode)" }
            sb.append("\"status\":\"").append(name).append("\",")
            sb.append("\"code\":").append(statusCode)
            if (tempC != null) sb.append(",")
        }
        if (tempC != null) {
            sb.append("\"max_temp_c\":").append(tempC)
        }
        sb.append("}")
        return sb.toString()
    }

    /**
     * 扫所有热区取最高温（摄氏度，保留一位小数）；一个都读不到返回 null。
     *
     * 【为什么要扫全部】不同厂商把 CPU/GPU/电池放在不同 zone，只读 zone0
     * 可能拿到的是主板温度而不是热点温度。
     */
    private fun maxThermalZoneTempC(): Double? {
        val base = File("/sys/class/thermal")
        val zones = base.listFiles { f -> f.name.startsWith("thermal_zone") } ?: return null
        var max = Double.MIN_VALUE
        var found = false
        for (zone in zones) {
            val raw = runCatching { File(zone, "temp").readText().trim() }.getOrNull() ?: continue
            val milli = raw.toLongOrNull() ?: continue
            // 有些设备直接给摄氏度（40），有些给毫摄氏度（40000）——按量级判断。
            val celsius = if (milli > 1000) milli / 1000.0 else milli.toDouble()
            if (celsius in 1.0..150.0) {
                if (celsius > max) max = celsius
                found = true
            }
        }
        if (!found) return null
        return Math.round(max * 10.0) / 10.0
    }
}
