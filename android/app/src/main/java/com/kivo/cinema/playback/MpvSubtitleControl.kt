package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.mpv.MpvTrackCatalog
import com.kivo.cinema.playback.subtitle.SubtitleSecondaryControl
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

    /**
     * 关闭字幕 —— 主位与次位**一起**关。
     *
     * 【为什么不能只写 `sid=no`】mpv 的次字幕是独立轨位：真机 P9 复验实测，
     * 次字幕在用时点「关闭字幕」只发 `sid="no"`，次字幕那一行照旧显示
     * （`secondary-sid="1"` 留着）—— 用户看到的是"关了还在"。
     * 「关闭字幕」是产品语义（一条都不显示），所以次位有值也一并清。
     *
     * @return true = 两个轨位都已关门（次位本来就没开也算成功）。
     */
    fun disableSubtitles(): Boolean {
        val mainOk = tracks.disableTrack("sub")
        val secondary = session.property("secondary-sid")?.trim()?.toIntOrNull()
        val secondaryOk = secondary == null || session.setProperty("secondary-sid", "no")
        if (!mainOk) reportError("关闭字幕失败")
        else if (!secondaryOk) reportError("次字幕未能关闭（id=$secondary）")
        return mainOk && secondaryOk
    }

    /**
     * 卸载一条外挂字幕（P9 · H）。
     *
     * 【删除当前主轨之后的兜底】`sub-remove` 不保证 mpv 会自动选下一个：
     * 读回 `sid` —— 仍是已删 id 或已无选择时，选剩余第一条；一条都不剩就显式
     * `sid=no`。兜底本身也失败时**回传给调用方**（[RemovalResult.warning]），
     * 不让界面停在"有轨但看不见"的暧昧态，也不再静默吞掉失败。
     *
     * 【为什么兜底要排除次字幕轨】主次同轨会叠印（重影）。若剩下的唯一候选
     * 正是 `secondary-sid`，选它等于制造重影 ⇒ 直接 `sid=no`。
     *
     * @return [RemovalResult.removed] = `sub-remove` 是否成功；
     *   [RemovalResult.warning] = 移除成功但兜底切换/关闭失败的原因（成功为 null）。
     */
    fun removeExternalSubtitle(trackId: Int): RemovalResult {
        if (!tracks.removeExternalSubtitle(trackId)) {
            reportError("删除字幕失败（id=$trackId）")
            return RemovalResult(removed = false, warning = null)
        }
        // 取证埋点已迁到调用方：`SubtitlePanelActions` 的 `sub-remove ok` 行（走 smbTrace）。
        // 原因：本机 MIUI logcat 全空，`android.util.Log` 在真机读不到（p9 家族树 §6.6）；
        // 且 playback 层不该反向依赖 UI 层的 smbTrace。
        var warning: String? = null
        val current = tracks.currentTrackId("sub")
        if (current == trackId || current < 0) {
            val secondaryId = SubtitleSecondaryControl.currentId(session)
            val remaining = tracks.subtitleTracks()
                .firstOrNull { it.id != trackId && it.id != secondaryId }
            if (remaining != null) {
                if (!tracks.selectTrack("sub", remaining.id)) warning = "删除后切换字幕失败"
            } else if (!tracks.disableTrack("sub")) {
                warning = "删除后关闭字幕失败"
            }
        }
        if (warning != null) reportError(warning)
        return RemovalResult(removed = true, warning = warning)
    }

    /** 卸载结果：轨道是否真的移除 + 兜底失败原因（UI 决定怎么提示）。 */
    data class RemovalResult(val removed: Boolean, val warning: String?)

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
