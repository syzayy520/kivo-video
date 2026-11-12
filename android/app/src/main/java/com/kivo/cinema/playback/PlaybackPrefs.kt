package com.kivo.cinema.playback

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import java.util.Locale

/**
 * 播放策略的唯一真值源（262 播放设置卡）。
 *
 * 【为什么归 playback 域而不是 settings UI 域】
 * 这些偏好既被设置页编辑，又被播放器内核在起播/seek 时读取。策略属于播放域，
 * UI 只是它的一个编辑面 —— 放 UI 域会让播放器反向依赖界面，破坏单向依赖。
 *
 * 【为什么不用 HomePreferences】那是首页展示域（autoplay）。播放策略是另一个
 * 变化原因，单独一个 prefs 文件，避免两域共用一个键空间互相覆盖。
 *
 * 默认值对齐参考设置页的出厂态；无后端的能力不在此建键（不伪造开关）。
 */
class PlaybackPrefs(context: Context) {

    private val app = context.applicationContext
    private val sp = app.getSharedPreferences("playback_prefs", Context.MODE_PRIVATE)

    /** 精确定位进度：seek 落到帧（exact）而非关键帧。 */
    var seekExact: Boolean
        get() = sp.getBoolean("seek_exact", false)
        set(v) { sp.edit().putBoolean("seek_exact", v).apply() }

    /** 显示剩余时长：底栏右侧时间显示「-剩余」而非总长。 */
    var showRemaining: Boolean
        get() = sp.getBoolean("show_remaining", false)
        set(v) { sp.edit().putBoolean("show_remaining", v).apply() }

    /** 起播时显示片源规格的停留秒数；0 = 不显示。 */
    var startupSpecSeconds: Int
        get() = sp.getInt("startup_spec_seconds", 3)
        set(v) { sp.edit().putInt("startup_spec_seconds", v).apply() }

    /** 拖动进度时显示预览画面。 */
    var scrubPreview: Boolean
        get() = sp.getBoolean("scrub_preview", true)
        set(v) { sp.edit().putBoolean("scrub_preview", v).apply() }

    /** 每次从头播放：true = 忽略续播位；false = 从上次进度续播。 */
    var alwaysFromStart: Boolean
        get() = sp.getBoolean("always_from_start", false)
        set(v) { sp.edit().putBoolean("always_from_start", v).apply() }

    /** 后台继续播放音频：false = 退后台暂停。 */
    var backgroundAudio: Boolean
        get() = sp.getBoolean("background_audio", false)
        set(v) { sp.edit().putBoolean("background_audio", v).apply() }

    /** 后台恢复时自动播放：回前台自动续播（仅当退后台时曾在播放）。 */
    var autoResume: Boolean
        get() = sp.getBoolean("auto_resume", false)
        set(v) { sp.edit().putBoolean("auto_resume", v).apply() }

    /** 自动开启画中画：按 Home 键离页时自动进 PiP。 */
    var autoPip: Boolean
        get() = sp.getBoolean("auto_pip", true)
        set(v) { sp.edit().putBoolean("auto_pip", v).apply() }

    /** 字幕首选语言（[Language] 的 code）；"system" = 跟随系统。 */
    var subtitleLanguage: String
        get() = sp.getString("subtitle_language", "system") ?: "system"
        set(v) { sp.edit().putString("subtitle_language", v).apply() }

    /** 忽略特效字幕样式：true = 剥离 ASS 特效按统一样式渲染。 */
    var ignoreAssStyle: Boolean
        get() = sp.getBoolean("ignore_ass_style", false)
        set(v) { sp.edit().putBoolean("ignore_ass_style", v).apply() }

    /** 单独调整播放亮度：true = 竖滑调窗口级亮度（退出还原）；false = 手势不调亮度。 */
    var independentBrightness: Boolean
        get() = sp.getBoolean("independent_brightness", true)
        set(v) { sp.edit().putBoolean("independent_brightness", v).apply() }

    /** 音频首选语言（[Language] code 或 "media"）；"media" = 媒体默认（不限制 alang）。 */
    var audioLanguage: String
        get() = sp.getString("audio_language", "media") ?: "media"
        set(v) { sp.edit().putString("audio_language", v).apply() }

    /** Wi-Fi 缓冲上限（MB）。 */
    var wifiBufferMb: Int
        get() = sp.getInt("wifi_buffer_mb", 2048)
        set(v) { sp.edit().putInt("wifi_buffer_mb", v).apply() }

    /** 蜂窝网络缓冲上限（MB）。 */
    var cellBufferMb: Int
        get() = sp.getInt("cell_buffer_mb", 1024)
        set(v) { sp.edit().putInt("cell_buffer_mb", v).apply() }

    // ---- 操控（264 操控设置卡）：按钮显隐 / 双击 / 竖滑 / 长按快进。播放器消费，UI 只编辑。 ----

    /** 底栏「下一集」按钮显隐（动作仍为剧集未接入的明示 toast）。 */
    var showNextButton: Boolean
        get() = sp.getBoolean("show_next_button", true)
        set(v) { sp.edit().putBoolean("show_next_button", v).apply() }

    /** 暂停态浮相机「截图」按钮显隐。 */
    var showCaptureButton: Boolean
        get() = sp.getBoolean("show_capture_button", true)
        set(v) { sp.edit().putBoolean("show_capture_button", v).apply() }

    /** 双击右侧快进步长（秒）。 */
    var seekForwardSeconds: Int
        get() = sp.getInt("seek_forward_seconds", 15)
        set(v) { sp.edit().putInt("seek_forward_seconds", v).apply() }

    /** 双击左侧快退步长（秒）。 */
    var seekBackwardSeconds: Int
        get() = sp.getInt("seek_backward_seconds", 15)
        set(v) { sp.edit().putInt("seek_backward_seconds", v).apply() }

    /** 双击中间动作："playpause" = 播放/暂停；"none" = 无。 */
    var doubleTapCenterAction: String
        get() = sp.getString("double_tap_center", "playpause") ?: "playpause"
        set(v) { sp.edit().putString("double_tap_center", v).apply() }

    /** 双击两侧动作："seek" = 快进/快退；"none" = 无。 */
    var doubleTapSidesAction: String
        get() = sp.getString("double_tap_sides", "seek") ?: "seek"
        set(v) { sp.edit().putString("double_tap_sides", v).apply() }

    /** 全屏上下滑动调整音量和亮度。 */
    var verticalSwipeEnabled: Boolean
        get() = sp.getBoolean("vertical_swipe", true)
        set(v) { sp.edit().putBoolean("vertical_swipe", v).apply() }

    /** 锁定态是否仍允许长按和双击（false = 锁定后手势全禁）。 */
    var lockAllowGestures: Boolean
        get() = sp.getBoolean("lock_allow_gestures", true)
        set(v) { sp.edit().putBoolean("lock_allow_gestures", v).apply() }

    /** 起播默认播放速度（mpv speed）。 */
    var defaultSpeed: Double
        get() = sp.getFloat("default_speed", 1.0f).toDouble()
        set(v) { sp.edit().putFloat("default_speed", v.toFloat()).apply() }

    /** 长按快进总开关。 */
    var longPressEnabled: Boolean
        get() = sp.getBoolean("long_press_enabled", true)
        set(v) { sp.edit().putBoolean("long_press_enabled", v).apply() }

    /** 长按快进起步倍速。 */
    var longPressBaseSpeed: Double
        get() = sp.getFloat("long_press_base_speed", 2.0f).toDouble()
        set(v) { sp.edit().putFloat("long_press_base_speed", v.toFloat()).apply() }

    /** 长按快进时左右滑动调整速度。 */
    var longPressSwipeSpeed: Boolean
        get() = sp.getBoolean("long_press_swipe_speed", true)
        set(v) { sp.edit().putBoolean("long_press_swipe_speed", v).apply() }

    /** 长按快进时向上滑动锁定（松手不恢复）。 */
    var longPressSwipeLock: Boolean
        get() = sp.getBoolean("long_press_swipe_lock", true)
        set(v) { sp.edit().putBoolean("long_press_swipe_lock", v).apply() }

    /** 长按松手恢复："previous" = 长按前速度；"keep" = 保持快进速度。 */
    var longPressRestore: String
        get() = sp.getString("long_press_restore", "previous") ?: "previous"
        set(v) { sp.edit().putString("long_press_restore", v).apply() }

    /**
     * 当前网络应使用的 demuxer 缓冲上限（mpv `demuxer-max-bytes` 值，如 "2048MiB"）。
     * 读不到网络类型时按 Wi-Fi 上限（内网片源多在 Wi-Fi，取大值更稳）。
     */
    fun demuxerMaxBytes(): String {
        val mb = if (isCellular()) cellBufferMb else wifiBufferMb
        return "${mb}MiB"
    }

    private fun isCellular(): Boolean {
        val cm = app.getSystemService(ConnectivityManager::class.java) ?: return false
        val caps = cm.getNetworkCapabilities(cm.activeNetwork) ?: return false
        return caps.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)
    }

    /**
     * 字幕首选语言对应的 mpv `slang`（逗号分隔的 ISO 639 码列表）。
     * "system" 取设备语言；空串表示不限制（mpv 默认）。
     */
    fun subtitleSlang(): String {
        val code = subtitleLanguage
        if (code == "system") {
            val lang = Locale.getDefault().language
            return languageByCode(lang)?.slang ?: lang
        }
        return languageByCode(code)?.slang ?: ""
    }

    /**
     * 音频首选语言对应的 mpv `alang`。"media" = 不限制（媒体默认，返回空串）；
     * "system" 取设备语言；其余按 [Language.slang]。与字幕 slang 同一份语言数据。
     */
    fun audioAlang(): String {
        val code = audioLanguage
        if (code == "media") return ""
        if (code == "system") {
            val lang = Locale.getDefault().language
            return languageByCode(lang)?.slang ?: lang
        }
        return languageByCode(code)?.slang ?: ""
    }

    /** 字幕语言候选（设置页 stepper 与内核 slang 共用同一份数据）。 */
    data class Language(val code: String, val label: String, val slang: String)

    companion object {
        val LANGUAGES: List<Language> = listOf(
            Language("system", "跟随系统", ""),
            Language("zh", "中文", "zh,chi,zho"),
            Language("en", "英语", "en,eng"),
            Language("ja", "日语", "ja,jpn"),
            Language("ko", "韩语", "ko,kor"),
            Language("fr", "法语", "fr,fre,fra"),
            Language("de", "德语", "de,ger,deu"),
            Language("es", "西班牙语", "es,spa"),
            Language("ru", "俄语", "ru,rus"),
            Language("ar", "阿拉伯语", "ar,ara"),
        )

        fun languageByCode(code: String): Language? = LANGUAGES.firstOrNull { it.code == code }

        fun languageLabel(code: String): String = languageByCode(code)?.label ?: "跟随系统"

        /** 音频首选语言标签："media" = 媒体默认，其余复用字幕语言标签。 */
        fun audioLanguageLabel(code: String): String =
            if (code == "media") "媒体默认" else languageLabel(code)

        /** 起播规格停留秒数候选。 */
        val SPEC_SECONDS: List<Int> = listOf(0, 3, 5, 10)

        /** 缓冲上限候选（MB）。 */
        val BUFFER_MB: List<Int> = listOf(256, 512, 1024, 2048, 4096)

        fun bufferLabel(mb: Int): String = if (mb >= 1024) "${mb / 1024}GB" else "${mb}MB"

        fun specLabel(seconds: Int): String = if (seconds <= 0) "关闭" else "${seconds}秒"

        /** 双击快进/快退步长候选（秒）。 */
        val SEEK_SECONDS: List<Int> = listOf(5, 10, 15, 20, 30, 60)

        /** 默认播放速度候选（mpv speed）。 */
        val DEFAULT_SPEEDS: List<Double> = listOf(0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0)

        /** 长按起步倍速候选。 */
        val LONG_PRESS_SPEEDS: List<Double> = listOf(1.5, 2.0, 2.5, 3.0)

        fun seekLabel(seconds: Int): String = "${seconds}秒"

        fun speedLabel(value: Double): String = "${value}x"

        fun doubleTapCenterLabel(code: String): String =
            if (code == "none") "无" else "播放 / 暂停"

        fun doubleTapSidesLabel(code: String): String =
            if (code == "none") "无" else "快进 / 快退"

        fun longPressRestoreLabel(code: String): String =
            if (code == "keep") "保持快进速度" else "长按前速度"
    }
}