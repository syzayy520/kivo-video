package com.kivo.cinema.ui.subtitles.library

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 字幕库（用户 2026-11-25 指出缺失：已下载/已翻译必须可见可管）。
 *
 * 两个分组，真源即文件系统：
 * - 「AI 翻译成品」：filesDir/ai_subs 目录下的 .srt（TranslationJob.writeSrt 产物，
 *   filesDir 不被系统清理；删除 = 下次观看重新翻译）
 * - 「已下载字幕」：filesDir/subtitles 目录（在线下载 / 手动导入的落库文件；
 *   删除后不再参与自动挂载）
 */
@Composable
internal fun SubtitleLibraryDialog(onDismiss: () -> Unit) {
    val context = LocalContext.current
    val revision = remember { mutableIntStateOf(0) }
    val translated = remember(revision.intValue) {
        scan(File(context.filesDir, "ai_subs"), suffix = ".srt")
    }
    val downloaded = remember(revision.intValue) {
        scan(File(context.filesDir, "subtitles"), suffix = null)
    }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("字幕库", fontSize = 17.sp) },
        text = {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Group("AI 翻译成品", translated) {
                    revision.intValue++
                }
                Group("已下载字幕", downloaded) {
                    revision.intValue++
                }
            }
        },
        confirmButton = { TextButton(onClick = onDismiss) { Text("关闭") } },
    )
}

private fun scan(dir: File, suffix: String?): List<File> {
    val files = dir.listFiles()?.filter { it.isFile && (suffix == null || it.name.endsWith(suffix)) } ?: emptyList()
    return files.sortedByDescending { it.lastModified() }
}

@Composable
private fun Group(title: String, files: List<File>, onChanged: () -> Unit) {
    val pending = remember { mutableStateListOf<File>() }
    Text(
        "$title（${files.size} 份）",
        fontSize = 13.sp,
        style = MaterialTheme.typography.labelLarge,
        modifier = Modifier.padding(top = 6.dp, bottom = 2.dp),
    )
    if (files.isEmpty()) {
        Text("暂无", fontSize = 13.sp, modifier = Modifier.padding(bottom = 8.dp))
        return
    }
    files.forEach { file ->
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Column(Modifier.weight(1f)) {
                Text(displayName(file), fontSize = 13.sp, maxLines = 1)
                Text(
                    "${formatSize(file.length())} · ${formatDate(file.lastModified())}",
                    fontSize = 11.sp,
                )
            }
            IconButton(onClick = {
                if (file.delete()) {
                    pending.add(file)
                    onChanged()
                }
            }) {
                Icon(Icons.Filled.Delete, contentDescription = "删除 ${displayName(file)}")
            }
        }
    }
}

/// 去掉哈希/版本前缀的可读名：`<hash>-cue-assembler-v3.2-zh-CN.srt` →
/// `译文 <hash8>`；`<hash12>-Lowlife…chs.srt` → 原名。
private fun displayName(file: File): String {
    val name = file.name.removeSuffix(".srt")
    val plain = name.substringAfter('-', missingDelimiterValue = "")
    if (plain.contains("cue-assembler")) {
        return "译文 ${name.take(8)}"
    }
    return if (plain.isNotBlank() && plain.length < name.length) plain else name
}

private fun formatSize(bytes: Long): String = when {
    bytes >= 1 shl 20 -> "%.1f MB".format(bytes / 1048576.0)
    bytes >= 1 shl 10 -> "%.1f KB".format(bytes / 1024.0)
    else -> "$bytes B"
}

private fun formatDate(millis: Long): String =
    SimpleDateFormat("MM-dd HH:mm", Locale.getDefault()).format(Date(millis))