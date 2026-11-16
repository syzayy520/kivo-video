package com.kivo.cinema.ui.brand

import android.content.Context
import android.content.pm.PackageManager

/**
 * 312 · 启动动画的**播放策略**。
 *
 * ## 设计稿的规则
 * 「完整版只在**安装或更新后首次启动**、**距上次使用超过一天**的冷启动时播放，
 * 其余用热启动版。」
 *
 * ## 为什么策略是纯函数 + 一层薄存储
 * 判定逻辑（首装？隔天？）是**规则**，必须能复算 ⇒ 抽成 [decide] 纯函数并单测；
 * 「上次什么时候播的 / 装的是哪个版本」是**状态**，放 SharedPreferences。
 * 分开的理由：规则错了要能一眼看出，状态丢了只会退化成"少播一次"，不该拖垮规则。
 */
class StartupPolicy(context: Context) {

    private val appContext = context.applicationContext
    private val prefs = appContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** 当前安装的版本号（用于识别"安装或更新"）。 */
    private fun versionCode(): Long = try {
        @Suppress("DEPRECATION")
        appContext.packageManager.getPackageInfo(appContext.packageName, 0).versionCode.toLong()
    } catch (e: PackageManager.NameNotFoundException) {
        0L
    }

    /** 上次播放**完整版**动画的时间；0 = 从未播过。 */
    private fun lastFullAt(): Long = prefs.getLong(KEY_LAST_FULL_AT, 0L)

    /** 上次记录的应用版本号；与当前不同 ⇒ 是"更新后的首次启动"。 */
    private fun lastVersion(): Long = prefs.getLong(KEY_VERSION, -1L)

    /**
     * 决定这次冷启动播什么。
     *
     * @param firstRunOfVersion 是否"安装或更新后的首次启动"
     * @param lastFullShownAtMs 上次播完整版的时间（0 = 从未）
     * @param nowMs 当前时间
     */
    fun decide(firstRunOfVersion: Boolean, lastFullShownAtMs: Long, nowMs: Long): Plan {
        if (firstRunOfVersion) return Plan.FULL
        if (lastFullShownAtMs <= 0L) return Plan.FULL
        return if (nowMs - lastFullShownAtMs >= ONE_DAY_MS) Plan.FULL else Plan.WARM
    }

    /** 供外部（如设置开关）关闭动画用。 */
    var enabled: Boolean
        get() = prefs.getBoolean(KEY_ENABLED, true)
        set(value) = prefs.edit().putBoolean(KEY_ENABLED, value).apply()

    /** 当前这次冷启动应该播什么。 */
    fun planForColdStart(nowMs: Long): Plan {
        if (!enabled) return Plan.NONE
        return decide(lastVersion() != versionCode(), lastFullAt(), nowMs)
    }

    /** 记录"完整版已播"（同时记下版本号，下次同一版本就不算首装）。 */
    fun markFullShown(nowMs: Long) {
        prefs.edit()
            .putLong(KEY_LAST_FULL_AT, nowMs)
            .putLong(KEY_VERSION, versionCode())
            .apply()
    }

    /** 记录"本版本已启动过"（即使只播了热启动版，也算不是首装）。 */
    fun markVersionSeen() {
        if (lastVersion() != versionCode()) {
            prefs.edit().putLong(KEY_VERSION, versionCode()).apply()
        }
    }

    /** 播放计划。 */
    enum class Plan {
        /** 完整版（冷启动 2.3 s：三角 + KIVO 字标 + 光线 + 柔光）。 */
        FULL,

        /** 热启动版（0.8 s：只有三角）。 */
        WARM,

        /** 不播。 */
        NONE,
    }

    companion object {
        private const val PREFS = "kivo_startup_anim"
        private const val KEY_LAST_FULL_AT = "last_full_at"
        private const val KEY_VERSION = "version_code"
        private const val KEY_ENABLED = "enabled"

        /** 「超过一天」的阈值。 */
        const val ONE_DAY_MS = 24L * 60L * 60L * 1000L
    }
}
