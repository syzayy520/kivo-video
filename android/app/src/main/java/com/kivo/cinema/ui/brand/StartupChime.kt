package com.kivo.cinema.ui.brand

import android.content.Context
import android.media.AudioManager
import android.media.SoundPool

/**
 * 312 · 启动音效（设计稿的「chime」）。
 *
 * ## 声音本身
 * 设计稿的 `chime()` 是：
 * · 三个正弦 **98 / 196 / 294 Hz**（基频 + 两个泛音，幅度 1 / .5 / .2）；
 * · 过一个 **lowpass 900 Hz**；
 * · **40 ms 起音** + **1.1 s 指数衰减**；
 * · 音量 **0.16**（`vol: .16`），在 **0.70 s** 响起（`sfx: .7`）。
 *
 * Android 上没有 WebAudio，最省的做法是**预先把这段合成成 WAV**（`res/raw/brand_chime.wav`），
 * 运行时用 [SoundPool] 播。合成脚本见 `docs/evidence/p316-startup-chime/status.md`。
 * 之所以不实时合成：启动动画期间主线程已经很忙（2.3 s 内要推 138 帧），
 * 再叠一个振荡器合成不划算；而且 WAV 只有 121 KB。
 *
 * ## 三条硬约束（设计稿明确要求）
 * 1. **不抢占音频焦点** —— [SoundPool] 走 `STREAM_MUSIC` 混音，不调
 *    `requestAudioFocus`，用户正在听歌时不会被打断（只是叠一声轻响）；
 * 2. **遵循静音与勿扰** —— `ringerMode != NORMAL`（静音/震动）或媒体音量为 0 时不播；
 * 3. **可以失败** —— 加载失败、设备无声都只是"没声音"，绝不影响动画本身。
 */
class StartupChime(context: Context) {

    private val appContext = context.applicationContext
    private val pool = SoundPool.Builder().setMaxStreams(1).build()
    private var sampleId = 0
    private var loaded = false
    private var released = false

    init {
        pool.setOnLoadCompleteListener { _, _, status ->
            loaded = status == 0
        }
        // 音量在 play() 里给（设计稿 vol=.16）
        sampleId = pool.load(appContext, com.kivo.cinema.R.raw.brand_chime, 1)
    }

    /**
     * 如果当前条件允许就播一声。
     *
     * @return 是否真的播了（false = 被静音/勿扰/未加载挡住）
     */
    fun playIfAllowed(): Boolean {
        if (released || !loaded) return false
        val am = appContext.getSystemService(AudioManager::class.java) ?: return false
        // 静音 / 震动模式不播（"遵循静音与勿扰"）
        if (am.ringerMode != AudioManager.RINGER_MODE_NORMAL) return false
        // 媒体音量为 0 时不播（勿扰或用户静音媒体）
        if (am.getStreamVolume(AudioManager.STREAM_MUSIC) <= 0) return false
        return runCatching {
            pool.play(sampleId, VOLUME, VOLUME, 1, 0, 1f)
            true
        }.getOrDefault(false)
    }

    /** 释放（覆盖层移除时调）。重复调用安全。 */
    fun release() {
        if (released) return
        released = true
        runCatching { pool.release() }
    }

    private companion object {
        /** 设计稿 `vol: .16`。 */
        const val VOLUME = 0.16f
    }
}
