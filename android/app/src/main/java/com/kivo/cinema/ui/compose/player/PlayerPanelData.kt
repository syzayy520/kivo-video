package com.kivo.cinema.ui.compose.player

import com.kivo.cinema.playback.MpvPlaybackSession

/**
 * 播放页 v2 的**数据装配**（player-ui-v2.md）。
 *
 * ## 为什么单独一个文件
 * `PlayerComposeActivity` 已经 1300+ 行（会话生命周期 / Surface / 弹幕 / 投屏 / 取证钩子
 * 全在里面）。再往里塞"面板数据从哪来"会让它继续膨胀。
 * 把**面板数据装配**抽出来，Activity 只剩"点哪儿调哪儿"的接线。
 *
 * ## 为什么不是 UI 层自己算
 * 面板里的每一项都必须**来自内核的真实状态**（mpv 的 speed / aid / sid 属性），
 * 不能由 UI 猜 —— 否则面板显示的和实际播放的对不上（这正是 P6 那一类 bug 的温床）。
 * 所以取数在**有会话**的地方做（这里），UI 只负责把数据画出来。
 */
object PlayerPanelData {

    /** 倍速档位（mpv 支持 0.1~10，这里给播放器常用的几档）。 */
    val SPEEDS = listOf("0.5x", "0.75x", "1x", "1.25x", "1.5x", "1.75x", "2x")

    /** 画质比例档位（对应 mpv 的 `video-params` / `panscan` 的显示比例）。 */
    val QUALITIES = listOf("16:9", "4:3", "2.35:1", "1.85:1", "2.39:1")

    /**
     * 倍速面板。
     *
     * @param current 当前倍速（读回内核的真实值，不是 UI 记的）。
     *               null = 还不知道 ⇒ **不预置任何选中项**（不撒谎）。
     */
    fun speed(current: String?): List<PlayerOptionItem> =
        SPEEDS.map { speed ->
            PlayerOptionItem(
                title = speed,
                selected = speed == current,
                tag = speed,
            )
        }

    /**
     * 音轨面板。
     *
     * @param tracks 内核的音轨列表。
     * @param currentId 当前选中的音轨 id。
     */
    fun audioTracks(
        tracks: List<com.kivo.cinema.mpv.MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> =
        tracks.map { track ->
            PlayerOptionItem(
                title = track.displayName(track.id),
                subtitle = track.codec,
                selected = track.id == currentId,
                tag = track.id.toString(),
            )
        }

    /**
     * 字幕轨面板。
     *
     * @param tracks 内核的字幕轨列表。
     * @param currentId 当前选中的字幕轨 id（-1 = 未选）。
     */
    fun subtitleTracks(
        tracks: List<com.kivo.cinema.mpv.MpvTrackCatalog.Track>,
        currentId: Int,
    ): List<PlayerOptionItem> =
        tracks.map { track ->
            PlayerOptionItem(
                title = track.displayName(track.id),
                subtitle = track.codec,
                selected = track.id == currentId,
                tag = track.id.toString(),
            )
        }

    /**
     * 画质比例面板。
     *
     * @param current 当前比例（空 = 自动）。
     */
    fun qualities(current: String): List<PlayerOptionItem> =
        listOf(PlayerOptionItem(title = "自动", selected = current.isBlank(), tag = "")) +
            QUALITIES.map { q ->
                PlayerOptionItem(
                    title = q,
                    selected = q == current,
                    tag = q,
                )
            }

    /**
     * 把倍速档位字符串转成 mpv 要的数值。
     *
     * @return null 表示这个档位不合法（**不猜**，宁可不设）。
     */
    fun speedValue(label: String): Double? {
        val value = label.removeSuffix("x").toDoubleOrNull() ?: return null
        return if (value in 0.1..10.0) value else null
    }

    /**
     * 从内核读当前倍速（显示用）。
     *
     * @return 例如 "1.5x"；**读不到返回 null**（而不是编一个 "1x"）。
     *
     * 【为什么读不到就返回 null】面板是在**会话建立之前**就可能被打开的，
     * 此时报"1x"等于**撒谎** —— 用户会以为当前就是 1 倍速。
     * 返回 null ⇒ 面板不显示倍速行，或者由调用方决定怎么表达"未知"。
     */
    fun currentSpeedLabel(session: MpvPlaybackSession?): String? {
        val speed = session?.currentSpeedOrNull() ?: return null
        val text = if (speed % 1.0 == 0.0) speed.toInt().toString() else speed.toString()
        return "${text}x"
    }
}
