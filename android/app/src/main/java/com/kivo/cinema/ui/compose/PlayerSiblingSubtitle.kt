package com.kivo.cinema.ui.compose

import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.playback.MpvPlaybackSession
import java.io.File

/**
 * D12-B2 · 本地同名外挂字幕自动挂载。
 *
 * ## 为什么要补
 * 旧宿主 `ui/PlayerActivity` 会扫同目录 `movie.ass` / `movie.srt` 并自绘字幕；
 * 新宿主本地文件走 `http://127.0.0.1` 代理喂给 mpv，mpv 的 `sub-auto` 只看得到代理 URL
 * 所在目录，看不到真实兄弟文件 ⇒ 老用户「放个同名 srt 就能看」的习惯被打破。
 *
 * ## 怎么做
 * 首帧后显式 `sub-add "<path>" select`（与「导入字幕」同一个内核命令），只认文本字幕。
 * 图像字幕（PGS/VobSub）由内嵌轨承担，不在这里猜。
 *
 * ## 判据
 * `NetDiag` 落一行：无同名 / 已挂载 / 挂载失败（带内核 lastError）。
 * 不做静默：有文件但挂不上时用户能在字幕面板里看到仍然只有内嵌轨，日志里能查到原因。
 */
internal fun PlayerComposeActivity.attachSiblingSubtitle(
    session: MpvPlaybackSession,
    mediaPath: String?,
) {
    if (mediaPath.isNullOrBlank()) return
    val media = File(mediaPath)
    val dir = media.parentFile ?: return
    val base = media.name.substringBeforeLast('.', media.name)
    val sibling = SUBTITLE_EXTS
        .map { File(dir, "$base.$it") }
        .firstOrNull { it.isFile }
    if (sibling == null) {
        NetDiag.log("[sub] 无同名外挂字幕：$base")
        return
    }
    val ok = session.command("sub-add", "\"${sibling.absolutePath}\" select")
    NetDiag.log(
        "[sub] 同名外挂字幕${if (ok) "已挂载" else "挂载失败(${session.lastError})"}：${sibling.name}"
    )
}

/** 与旧宿主一致：只认文本字幕；顺序决定同名多格式时优先 ass。 */
private val SUBTITLE_EXTS = listOf("ass", "ssa", "srt", "vtt")
