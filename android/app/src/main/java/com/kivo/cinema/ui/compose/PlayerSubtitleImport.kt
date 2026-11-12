package com.kivo.cinema.ui.compose

import android.net.Uri
import com.kivo.cinema.playback.MpvPlaybackSession
import java.io.File

/**
 * 外挂字幕导入（player-ui-v3 §1.6）。
 *
 * 【为什么拷进缓存再 sub-add】SAF 给的是 content:// URI，mpv 只认文件路径；
 * 且 URI 的读取权限随这次选择而来，不拷一份，下次起播就找不到了。
 *
 * 【为什么 select】导入即启用；要换回内嵌轨，字幕面板里切。
 *
 * 【为什么不走 Intent 传路径】与 SMB 凭据同一条铁律：外部输入只给 URI，
 * 落盘名字由这里消毒，路径不暴露给任何第三方组件。
 */
internal fun PlayerComposeActivity.launchSubtitleImport() {
    runCatching { subtitleImportLauncher.launch(arrayOf("*/*")) }
        .onFailure { showToast("无法打开系统文件选择器：${it.message}") }
}

internal fun PlayerComposeActivity.importSubtitleFile(uri: Uri) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法导入字幕")
        return
    }
    Thread {
        val error = copyAndAddSubtitle(uri, mpv)
        runOnUiThread {
            if (error == null) {
                panelRevision++
                showToast("字幕已导入并启用")
            } else {
                showToast(error)
            }
        }
    }.start()
}

/** @return null＝成功；否则是给用户看的失败原因（非空、可见）。 */
private fun PlayerComposeActivity.copyAndAddSubtitle(uri: Uri, mpv: MpvPlaybackSession): String? {
    val dir = File(cacheDir, "subs").apply { mkdirs() }
    val target = File(dir, subtitleFileName(uri))
    return try {
        val copied = contentResolver.openInputStream(uri)?.use { input ->
            target.outputStream().use { input.copyTo(it) }
        }
        if (copied == null) return "无法读取所选文件"
        if (mpv.command("sub-add", "\"${target.absolutePath}\" select")) null
        else "字幕加载失败：${mpv.lastError}"
    } catch (e: Exception) {
        "字幕导入失败：${e.message}"
    }
}

/** 文件名来自 SAF：去掉路径分隔与保留字符，杜绝写出缓存目录之外。 */
private fun PlayerComposeActivity.subtitleFileName(uri: Uri): String {
    val raw = runCatching {
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            val index = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
            if (index >= 0 && cursor.moveToFirst()) cursor.getString(index) else null
        }
    }.getOrNull()
    val cleaned = raw?.replace(Regex("[\\\\/:*?\"<>|]"), "_")?.takeIf { it.isNotBlank() }
    return cleaned ?: "subtitle-${System.currentTimeMillis()}.srt"
}