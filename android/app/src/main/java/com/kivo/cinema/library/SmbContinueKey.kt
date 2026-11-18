package com.kivo.cinema.library

/**
 * SMB 续播键 `smb://<sourceId>/<share>/<path>` 的解析。
 *
 * 键的**写入格式**由播放侧 `PlayerSessionBridge.watchKey()` 定义（345 定稿，
 * 为的是消灭"所有 SMB 共用一键"的串台事故）；本对象只读不写，解析出打开播放所需的
 * sourceId/share/path，以及列表展示用的片名（文件名去扩展名）。
 */
object SmbContinueKey {

    data class Parts(val sourceId: String, val share: String, val path: String) {
        val fileName: String get() = path.substringAfterLast('/')
        /** 文件名去扩展名当片名；拿不到就退回原文件名。 */
        val title: String get() = fileName.substringBeforeLast('.').ifBlank { fileName }
    }

    fun parse(key: String): Parts? {
        if (!key.startsWith(PREFIX)) return null
        val rest = key.removePrefix(PREFIX)
        val sourceId = rest.substringBefore('/', "")
        val afterSource = rest.substringAfter('/', "")
        val share = afterSource.substringBefore('/', "")
        val path = afterSource.substringAfter('/', "")
        if (sourceId.isBlank() || path.isBlank()) return null
        return Parts(sourceId, share, path)
    }

    private const val PREFIX = "smb://"
}
