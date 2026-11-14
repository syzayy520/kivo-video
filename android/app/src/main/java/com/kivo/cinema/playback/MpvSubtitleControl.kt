package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.mpv.MpvTrackCatalog
import java.io.File

/**
 * 253-P6-2：字幕控制 —— **只做一件事**：把"用户想换字幕"翻译成 mpv 的属性/命令。
 *
 * ## 为什么单独一个类
 * 字幕是 mpv 里**语义最绕**的一块：
 * · 选轨用 `sid`、关轨用 `sid=no`（不是删轨）；
 * · mpv 有三种状态（没挂轨 / 挂了没选 / 选了且可见），
 *   中间那一种在界面上和"完全没字幕"长得一模一样；
 * · 外挂字幕**画进视频帧**（libass），不是另画一层。
 * 这些规则与"会话的生命周期"没关系，却与"内核的字幕语义"绑死
 * ⇒ 跟着内核语义走，不跟着会话走。
 *
 * ## 为什么交给 mpv 而不是自己画
 * mpv 的 `vo=libmpv` 会把字幕画进视频帧（渲染后端调
 * `gl_video_set_osd_source(vo->osd)`，osd 内部走 libass）。
 * 我们若再解析一遍再叠一层，等于两套字幕同时显示、两个位置。
 */
class MpvSubtitleControl(
    private val session: MpvSession,
    /** 失败原因回报（写进会话的 lastError，供上层显示）。 */
    private val reportError: (String) -> Unit,
) {

    private val tracks: MpvTrackCatalog get() = MpvTrackCatalog(session)

    /** 字幕是否已启用（mpv 的 `sid` 有值即为启用）。 */
    val enabled: Boolean get() = tracks.currentTrackId("sub") >= 0

    /**
     * 挂载一条外挂字幕文件。
     *
     * @param file 本地字幕文件（.srt / .ass / .ssa）。
     * @return true = 挂载成功。
     */
    fun loadExternalSubtitle(file: File): Boolean {
        if (!file.isFile) {
            reportError("字幕文件不存在：${file.name}")
            return false
        }
        val ok = tracks.addExternalSubtitle(file.absolutePath)
        if (!ok) {
            reportError("字幕挂载失败：${file.name}")
        }
        return ok
    }

    /**
     * 选中一条字幕轨。
     *
     * @param trackId 轨道 id（来自 [MpvTrackCatalog.tracks]）。
     */
    fun selectSubtitle(trackId: Int): Boolean {
        val ok = tracks.selectTrack("sub", trackId)
        if (!ok) reportError("字幕轨切换失败（id=$trackId）")
        return ok
    }

    /** 关闭字幕。 */
    fun disableSubtitles(): Boolean {
        val ok = tracks.disableTrack("sub")
        if (!ok) reportError("关闭字幕失败")
        return ok
    }

    /**
     * 当前字幕轨是否在**显示**。
     *
     * 【为什么不能只看"选了没有"】
     * mpv 有三种状态：没挂载字幕轨、挂了但没选、选了且可见。
     * 中间那一种（挂了但没选）在界面上和"完全没字幕"是一样的 ——
     * 都看不见，但前者可以一键打开，后者要先挂文件。
     */
    fun isSubtitleVisible(): Boolean {
        val id = tracks.currentTrackId("sub")
        if (id < 0) return false
        // mpv 的 `sub-visibility` 默认是 yes；显式问一次比假设更可靠。
        return session.property("sub-visibility") != "no"
    }
}
