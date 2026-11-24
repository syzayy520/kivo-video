package com.kivo.cinema.ui.subtitles.online

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.clickable
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import com.kivo.cinema.playback.subtitle.RemoteSubtitle
import com.kivo.cinema.subtitle.online.OnlineSubtitle
import com.kivo.cinema.subtitle.online.OnlineSubtitleDownloader
import com.kivo.cinema.subtitle.online.OnlineSubtitlePrefs
import com.kivo.cinema.subtitle.online.OnlineSubtitleRepository
import com.kivo.cinema.subtitle.online.OnlineSubtitleTranslator
import com.kivo.cinema.subtitle.online.OpenSubtitlesClient
import com.kivo.cinema.subtitle.online.SubHdClient
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.ui.compose.showToast
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.subtitles.online.OnlineSubtitleResultList
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

/**
 * 在线字幕面板内容层（播放薄壳与详情页壳共用，design.md §八 决策）：
 * 搜索 → 结果 → 下载/翻译 → 字幕库。
 *
 * [autoAttach]=true（播放内）：下载/翻译完成即调 [onAttach] 挂载当前播放。
 * [autoAttach]=false（详情页）：只落库 + [videoKey] 非空时预登记（起播自动挂载）。
 * 翻译经 [OnlineSubtitleTranslator] 异步执行，[onProgressText] 反映分钟级进度（不再无响应）。
 */
@Composable
internal fun OnlineSubtitleSheetContent(
    context: android.content.Context,
    initialQuery: String,
    year: Int?,
    videoKey: String?,
    autoAttach: Boolean,
    onAttach: ((com.kivo.cinema.playback.subtitle.SubtitleImportStore.Imported) -> Unit)?,
    onDismiss: () -> Unit,
) {
    val prefs = remember { OnlineSubtitlePrefs(context) }
    val repo = remember {
        val tmdb = TmdbClient(context)
        OnlineSubtitleRepository(OpenSubtitlesClient(tmdb, prefs), SubHdClient(tmdb), context)
    }
    val downloader = remember {
        OnlineSubtitleDownloader(OpenSubtitlesClient(TmdbClient(context), prefs), SubHdClient(TmdbClient(context)),
            File(context.cacheDir, "subtitles-remote")
        )
    }
    val store = remember { SubtitleImportStore(context) { /* 详情页无 smbTrace */ } }
    val scope = rememberCoroutineScope()
    var query by remember { mutableStateOf(initialQuery) }
    var items by remember { mutableStateOf<List<OnlineSubtitle>>(emptyList()) }
    var loading by remember { mutableStateOf(false) }
    var notice by remember { mutableStateOf<String?>(null) }
    var busyId by remember { mutableStateOf<String?>(null) }
    var progressText by remember { mutableStateOf<String?>(null) }
    var searched by remember { mutableStateOf(false) }

    fun importRemote(remote: RemoteSubtitle, titleOverride: String? = null) {
        val imported = store.importFile(remote.file, titleOverride ?: remote.title)
        if (imported == null) {
            context.let { android.widget.Toast.makeText(it, "字幕导入失败：无法保存下载的字幕", android.widget.Toast.LENGTH_SHORT).show() }
            return
        }
        if (autoAttach) {
            onAttach?.invoke(imported)
            context.let { android.widget.Toast.makeText(it, "字幕已导入并启用", android.widget.Toast.LENGTH_SHORT).show() }
        } else {
            if (videoKey != null) { store.remember(videoKey, imported.file) }
            context.let { android.widget.Toast.makeText(it, "字幕已下载，播放时自动挂载", android.widget.Toast.LENGTH_SHORT).show() }
        }
    }

    fun download(item: OnlineSubtitle) {
        busyId = "${item.source}:${item.id}"
        progressText = null
        scope.launch {
            val outcome = runCatching { withContext(Dispatchers.IO) { downloader.download(item) } }
            busyId = null
            outcome.onSuccess { importRemote(it) }
                .onFailure { progressText = "下载失败：${it.message ?: "未知错误"}" }
        }
    }

    fun translate(item: OnlineSubtitle) {
        busyId = "${item.source}:${item.id}"
        progressText = "翻译准备中…（字幕多时需数分钟，可离开本页稍后回来）"
        scope.launch {
            val outcome = runCatching {
                withContext(Dispatchers.IO) {
                    val remote = downloader.download(item)
                    val text = remote.file.readText()
                    val translator = OnlineSubtitleTranslator(context)
                    translator.translateToChinese(text, 0L) { progress ->
                        progressText = "翻译中 ${progress.processedCues}/${progress.totalCues} 条…"
                    }
                }
            }
            busyId = null
            outcome.onSuccess { result ->
                val path = result.translationPath
                if (path.isNullOrBlank()) {
                    progressText = result.reason ?: "翻译未完成，请检查模型与网络"
                    return@onSuccess
                }
                importRemote(com.kivo.cinema.playback.subtitle.RemoteSubtitle(File(path), item.title + "（中文）"))
            }.onFailure {
                progressText = "翻译失败：${it.message ?: "未知错误"}"
            }
        }
    }

    Column(Modifier.fillMaxSize().padding(start = 16.dp, end = 16.dp, top = 12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("在线字幕", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                modifier = Modifier.weight(1f))
            Text("关闭", fontSize = 14.sp, color = DesignTokens.textSub,
                modifier = Modifier.padding(8.dp).clickable(onClick = onDismiss))
        }
        Spacer(Modifier.height(10.dp))
        OutlinedTextField(
            value = query, onValueChange = { query = it }, singleLine = true,
            placeholder = { Text("片名、年份（可选）", fontSize = 13.sp) },
            modifier = Modifier.fillMaxWidth(),
            shape = RoundedCornerShape(12.dp),
        )
        Spacer(Modifier.height(8.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp), verticalAlignment = Alignment.CenterVertically) {
            Button(onClick = {
                if (query.isBlank() || loading) return@Button
                loading = true; searched = true; notice = null
                scope.launch {
                    val result = withContext(Dispatchers.IO) { repo.search(query.trim(), year) }
                    items = result.items
                    notice = result.notice
                    loading = false
                }
            }, enabled = !loading && query.isNotBlank()) {
                Text(if (loading) "搜索中…" else "搜索")
            }
        }
        // notice 独立一行：塞在搜索行旁会被长文本挤压不可见（真机谷雨案例）。
        if (notice != null) Text(
            notice ?: "", fontSize = 11.sp, color = DesignTokens.textSub,
            modifier = Modifier.fillMaxWidth().padding(top = 6.dp),
        )
        Spacer(Modifier.height(12.dp))
        when {
            loading -> Box(Modifier.fillMaxWidth().padding(32.dp), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(color = DesignTokens.textSub)
            }
            progressText != null -> Column(
                Modifier.fillMaxWidth().padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                CircularProgressIndicator(color = DesignTokens.textSub, modifier = Modifier.size(28.dp))
                Spacer(Modifier.height(14.dp))
                Text(progressText ?: "", fontSize = 14.sp, color = DesignTokens.textSub,
                    modifier = Modifier.fillMaxWidth())
            }
            searched && items.isEmpty() -> Text(
                "没有找到相关字幕，换个片名或加上年份试试", fontSize = 13.sp, color = DesignTokens.textSub,
            )
            else -> OnlineSubtitleResultList(items, busyId, ::download, ::translate)
        }
    }
}

/** 面板骨架（遮罩 + 圆角卡），内容由 [content] 提供；播放薄壳与详情页壳共用。 */
@Composable
internal fun OnlineSubtitleSheetFrame(onDismiss: () -> Unit, content: @Composable () -> Unit) {
    Box(Modifier.fillMaxSize().background(DesignTokens.textMain.copy(alpha = .5f))) {
        Column(
            Modifier.fillMaxSize().padding(top = 36.dp)
                .background(DesignTokens.pageBackground, RoundedCornerShape(topStart = 24.dp, topEnd = 24.dp))
                .clickable(enabled = false) {},
        ) {
            content()
        }
    }
}