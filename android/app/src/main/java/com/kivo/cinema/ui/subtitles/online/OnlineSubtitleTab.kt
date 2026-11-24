package com.kivo.cinema.ui.subtitles.online

import androidx.compose.foundation.layout.Arrangement
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
import com.kivo.cinema.subtitle.online.OnlineSubtitle
import com.kivo.cinema.subtitle.online.OnlineSubtitleDownloader
import com.kivo.cinema.subtitle.online.OnlineSubtitlePrefs
import com.kivo.cinema.subtitle.online.OnlineSubtitleRepository
import com.kivo.cinema.subtitle.online.OnlineSubtitleTranslator
import com.kivo.cinema.subtitle.online.OpenSubtitlesClient
import com.kivo.cinema.subtitle.online.SubHdClient
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.attachImported
import com.kivo.cinema.ui.compose.currentTitle
import com.kivo.cinema.ui.compose.showToast
import com.kivo.cinema.ui.compose.subtitleSourceKey
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.compose.smbTrace
import com.kivo.cinema.ui.history.glassSurface
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

/**
 * 在线字幕面板（播放「搜索字幕」入口）：搜索 → 结果 → 下载/翻译 → 挂载。
 * 数据全走 subtitle/online 数据族；挂载复用 attachImported（播放只读）。
 */
@Composable
internal fun PlayerComposeActivity.OnlineSubtitleTab(onClose: () -> Unit) {
    val context = this
    val prefs = remember { OnlineSubtitlePrefs(context) }
    val repo = remember {
        val tmdb = TmdbClient(this)
        OnlineSubtitleRepository(OpenSubtitlesClient(tmdb, prefs), SubHdClient(tmdb))
    }
    val downloader = remember {
        OnlineSubtitleDownloader(OpenSubtitlesClient(TmdbClient(this), prefs), SubHdClient(TmdbClient(this)),
            File(cacheDir, "subtitles-remote")
        )
    }
    val scope = rememberCoroutineScope()
    var query by remember { mutableStateOf(currentTitle()) }
    var items by remember { mutableStateOf<List<OnlineSubtitle>>(emptyList()) }
    var loading by remember { mutableStateOf(false) }
    var notice by remember { mutableStateOf<String?>(null) }
    var busyId by remember { mutableStateOf<String?>(null) }
    var searched by remember { mutableStateOf(false) }

    fun search() {
        if (query.isBlank() || loading) return
        loading = true; searched = true; notice = null
        scope.launch {
            val result = withContext(Dispatchers.IO) { repo.search(query.trim()) }
            items = result.items
            notice = result.notice
            loading = false
        }
    }
    LaunchedEffect(Unit) { search() }

    fun importRemote(remote: com.kivo.cinema.playback.subtitle.RemoteSubtitle) {
        val imported = SubtitleImportStore(this) { smbTrace(it) }.importFile(remote.file, remote.title)
        if (imported == null) {
            showToast("字幕导入失败：无法保存下载的字幕")
            return
        }
        if (prefs.autoSelect) {
            val failure = attachImported(imported, requireNotNull(mpvNetSession))
            if (failure != null) {
                showToast(failure)
                return
            }
            panelRevision++
        } else {
            SubtitleImportStore(this) { smbTrace(it) }.remember(subtitleSourceKey(), imported.file)
        }
        showToast("字幕已导入并启用")
    }

    fun download(item: OnlineSubtitle) {
        busyId = "${item.source}:${item.id}"
        scope.launch {
            val outcome = runCatching { withContext(Dispatchers.IO) { downloader.download(item) } }
            busyId = null
            outcome.onSuccess { importRemote(it) }
                .onFailure { showToast(it.message ?: "下载失败") }
        }
    }

    fun translate(item: OnlineSubtitle) {
        busyId = "${item.source}:${item.id}"
        scope.launch {
            val outcome = runCatching {
                withContext(Dispatchers.IO) {
                    val remote = downloader.download(item)
                    val text = remote.file.readText()
                    OnlineSubtitleTranslator(this@OnlineSubtitleTab).translateToChinese(text)
                }
            }
            outcome.onSuccess { result ->
                busyId = null
                val path = result.translationPath
                if (path.isNullOrBlank()) {
                    showToast("翻译未完成，请检查模型与网络")
                    return@onSuccess
                }
                importRemote(com.kivo.cinema.playback.subtitle.RemoteSubtitle(
                    File(path), item.title + "（中文）"))
            }.onFailure {
                busyId = null
                showToast(it.message ?: "翻译失败")
            }
        }
    }

    Column(Modifier.fillMaxSize().padding(start = 16.dp, end = 16.dp, top = 12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("在线字幕", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                modifier = Modifier.weight(1f))
            Text("关闭", fontSize = 14.sp, color = DesignTokens.textSub,
                modifier = Modifier.padding(8.dp).clickable(onClick = onClose))
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
            Button(onClick = { search() }, enabled = !loading && query.isNotBlank()) {
                Text(if (loading) "搜索中…" else "搜索")
            }
            if (notice != null) Text(notice ?: "", fontSize = 11.sp, color = DesignTokens.textSub,
                modifier = Modifier.weight(1f))
        }
        Spacer(Modifier.height(12.dp))
        when {
            loading -> Box(Modifier.fillMaxWidth().padding(32.dp), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(color = DesignTokens.textSub)
            }
            searched && items.isEmpty() -> Text(
                "没有找到相关字幕，换个片名或加上年份试试", fontSize = 13.sp, color = DesignTokens.textSub,
            )
            else -> OnlineSubtitleResultList(items, busyId, ::download, ::translate)
        }
    }
}