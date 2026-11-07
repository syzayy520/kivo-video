package com.kivo.cinema.download

/** 下载任务数据类（225 设计稿 D1）。 */
data class DownloadEntry(
    val url: String,
    val localPath: String,
    val title: String,
    var progress: Int = 0,      // 0-100
    var downloadedBytes: Long = 0,
    var totalBytes: Long = 0,
    var state: State = State.PENDING,
) {
    enum class State { PENDING, DOWNLOADING, COMPLETED, FAILED }
}
