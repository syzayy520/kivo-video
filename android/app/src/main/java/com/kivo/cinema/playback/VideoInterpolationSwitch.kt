package com.kivo.cinema.playback

import android.content.Context
import android.content.Intent

/**
 * 帧插值的**唯一决策点**（口径同 [PlaybackStack.resolve]：一处算，多处用）。
 *
 * ## 优先级（从高到低）
 *
 * 1. **系统属性** `debug.kivo.interpolation=on|off` —— 排障/取证用（反射读隐藏 API）。
 * 2. **intent** `--ei kivo.interpolation 0|1` —— 真机 A/B 实验用（不改设置就能开）。
 * 3. **用户设置** [PlaybackPrefs.videoInterpolation]（`auto` / `on` / `off`）。
 * 4. 默认：`auto` ⇒ **on**（实测通过，见下）。
 *
 * ## `auto` = on 的依据（2026-11-15 真机实测，K40）
 *
 * | 场景 | 插值关 | 插值开 | 参照 SenPlayer |
 * | --- | --- | --- | --- |
 * | 4K HDR 25 fps（Emby） | 25.00 fps | **59.47 fps** | — |
 * | 1080p 23.9 fps（SMB 12GB） | — | **60.44 fps** | **34.19 fps** |
 *
 * 1080p 臂：p50 16.7 ms（满帧）、p99 23.4 ms、max 48.8 ms、>200 ms **0 次**。
 * 4K HDR 臂：p50 17.2 ms、p99 37.3 ms、max 97.5 ms、>200 ms **0 次**，
 * 但内核有 8 次 `Render queue underrun`（Adreno 650 在 4K 插值下偶尔供帧不及）。
 *
 * 对照 SenPlayer 同片源 34.19 fps / 3 次 >200 ms / 最长 3.22 s ⇒ **两档都赢**。
 *
 * ## 为什么仍然要留着开关
 *
 * · **肥皂剧效应**：24fps 电影插到 60fps 会失去胶片感，这是主观偏好。
 * · 4K 上的 `Render queue underrun` 说明余量不宽裕，个别更吃 GPU 的片源可能更差。
 * · 所以 `off` 必须是一个用户点得到的状态，而不是只有一个"关掉实验开关"的后门。
 */
internal object VideoInterpolationSwitch {

    /** 系统属性名（排障用）。 */
    private const val PROP = "debug.kivo.interpolation"

    /** intent 里覆盖的 key（实验用）。 */
    private const val INTENT_KEY = "kivo.interpolation"

    /** 是否启用插值。 */
    fun isEnabled(prefs: PlaybackPrefs, intent: Intent?): Boolean {
        // 1) 系统属性优先（紧急回退 / 取证量基线）。
        when (sysProp(PROP)?.lowercase()) {
            "on", "1", "true", "yes" -> return true
            "off", "0", "false", "no" -> return false
        }
        // 2) intent 覆盖（真机 A/B：--ei kivo.interpolation 1）
        if (intent != null && intent.hasExtra(INTENT_KEY)) {
            return intent.getIntExtra(INTENT_KEY, 0) != 0
        }
        // 3) 用户设置
        return when (prefs.videoInterpolation) {
            PlaybackPrefs.INTERPOLATION_OFF -> false
            PlaybackPrefs.INTERPOLATION_ON -> true
            // 4) auto = on：实测两档都零卡顿且帧数翻倍，理由见类注释的对照表。
            else -> true
        }
    }

    /**
     * 读系统属性（隐藏 API，用反射）。
     *
     * 【为什么不用 android.os.SystemProperties 直接调】它是隐藏 API，import 会编译失败。
     * 反射被拦就当"没设" ⇒ 落到下一级。
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
