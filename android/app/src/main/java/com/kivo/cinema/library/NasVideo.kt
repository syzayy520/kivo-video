package com.kivo.cinema.library

/**
 * NAS 片库里的一条影片（索引条目，不含 UI/网络）。
 *
 * [path] 是共享内路径（如 `电影/[死亡实验]…/xxx.mkv`），与 [sourceId]/[share] 一起
 * 才能还原成播放所需的 `smb://` 坐标；[addedMs] 取扫描时的 mtime，用于"最近添加"排序。
 * [category] 由所属根目录决定（不是内容识别）；[posterPath] 是同目录封面，可能为空。
 */
data class NasVideo(
    val sourceId: String,
    val share: String,
    val path: String,
    val size: Long,
    val addedMs: Long,
    val category: NasCategory,
    /** 同目录封面（共享内路径）；null = 库里没图，上层兜底 TMDB。 */
    val posterPath: String? = null,
) {
    /** 展示用文件名（路径最后一段）。 */
    val fileName: String get() = path.substringAfterLast('/')

    /** 续播/播放键：与 `PlayerSessionBridge.watchKey()` 同格式，天然对齐。 */
    val watchKey: String get() = "smb://$sourceId/$share/$path"
}
