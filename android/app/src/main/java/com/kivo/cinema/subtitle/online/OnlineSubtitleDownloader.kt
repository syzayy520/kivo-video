package com.kivo.cinema.subtitle.online

import com.kivo.cinema.playback.subtitle.RemoteSubtitle
import com.kivo.cinema.playback.subtitle.SubtitleRemoteFetcher
import java.io.File

/**
 * 在线字幕下载（design.md §四）：
 * OpenSubtitles → /download 取 link → SubtitleRemoteFetcher（8MiB/HTML 拒收/哈希去重）。
 * SubHD → prepare-download 的 /down/ 链接；Cloudflare 拦截/返回 HTML 时抛可见原因。
 */
internal class OnlineSubtitleDownloader(
    private val openSubtitles: OpenSubtitlesClient,
    private val subHd: SubHdClient,
    private val cacheDir: File,
) {
    fun download(subtitle: OnlineSubtitle): RemoteSubtitle {
        val url = when (subtitle.source) {
            SubSource.OpenSubtitles -> openSubtitles.downloadLink(subtitle.fileId)
            SubSource.SubHd -> subHd.prepareDownload(subtitle)
        }
        return SubtitleRemoteFetcher.fetch(url, cacheDir).getOrElse { throw it }
    }
}