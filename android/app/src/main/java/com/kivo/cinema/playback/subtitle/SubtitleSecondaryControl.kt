package com.kivo.cinema.playback.subtitle

import com.kivo.cinema.mpv.MpvSession

/**
 * 次字幕语义（P9 · F）。只做一件事：把"主/次两条字幕"的互斥规则翻成属性写。
 *
 * mpv 的次字幕是**独立轨位**：`secondary-sid` 与 `sid` 各选一条，
 * 位置（`secondary-sub-pos`）、显隐（`secondary-sub-visibility`）、
 * 延迟（`secondary-sub-delay`）都独立。主次不允许同一条轨 ——
 * 同轨会让同一句字幕在两个位置叠印（重影）。
 */
internal object SubtitleSecondaryControl {

    /** 当前次字幕轨 id；没选/auto ⇒ null（不猜）。 */
    fun currentId(native: MpvSession?): Int? =
        native?.property("secondary-sid")?.trim()?.toIntOrNull()

    /**
     * 选次字幕。
     *
     * @return null = 成功；否则是给用户看的原因。
     */
    fun select(native: MpvSession, trackId: Int, mainId: Int): String? {
        if (trackId == mainId) return "次字幕不能与主字幕是同一条轨道"
        return if (native.setProperty("secondary-sid", trackId.toString())) null
        else "次字幕切换失败（内核拒绝 id=$trackId）"
    }

    fun disable(native: MpvSession): Boolean = native.setProperty("secondary-sid", "no")

    fun setVisible(native: MpvSession, visible: Boolean): Boolean =
        native.setProperty("secondary-sub-visibility", if (visible) "yes" else "no")
}
