package com.kivo.cinema.library

/** 本地视频条目（222 设计稿 D1）：MediaStore 查询结果映射。 */
data class MediaItem(
    val id: Long,
    val title: String,
    val path: String,
    val durationMs: Long,
    val width: Int,
    val height: Int,
    val addedSec: Long = 0,     // DATE_ADDED（236：媒体库日期行/发现页排序用）
) {
    fun formatDuration(): String {
        val s = durationMs / 1000
        return "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60)
    }
}
