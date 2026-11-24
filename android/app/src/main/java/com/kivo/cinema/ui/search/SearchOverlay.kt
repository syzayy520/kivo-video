package com.kivo.cinema.ui.search

import android.content.ActivityNotFoundException
import android.content.Intent
import android.speech.RecognizerIntent
import android.widget.Toast
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.search.BrowseRepository
import com.kivo.cinema.search.CategoryArtworkRepository
import com.kivo.cinema.search.HotSearchRepository
import com.kivo.cinema.search.RecentSearchStore
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.search.SearchRepository
import com.kivo.cinema.ui.motion.MotionGate
import com.kivo.cinema.ui.search.browse.SearchBrowsePage
import com.kivo.cinema.ui.search.home.SearchHomePage
import com.kivo.cinema.ui.search.query.SearchQueryPage

private enum class SearchMode { Home, Query, Browse }

/**
 * 搜索覆盖层宿主（family-tree §三）：① 首页 / ② 输入中 / ③④ 分类筛选三态切换 + 返回键 +
 * 底栏 slot（输入中不渲染底栏）+ 语音识别入口。数据态全在仓储，本文件只做编排。
 */
@Composable
internal fun SearchOverlay(
    recent: RecentSearchStore,
    search: SearchRepository,
    hot: HotSearchRepository,
    artwork: CategoryArtworkRepository,
    browse: BrowseRepository,
    onOpenEntry: (SearchEntry) -> Unit,
    onClose: () -> Unit,
    navSlot: @Composable () -> Unit,
) {
    var mode by rememberSaveable { mutableStateOf(SearchMode.Home) }
    var recentVersion by remember { mutableStateOf(0) }
    val reduce = MotionGate.rememberAppReduceMotion()
    val context = LocalContext.current
    val duration = if (reduce) 0 else 200

    val voice = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        val text = result.data?.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS)?.firstOrNull()
        if (!text.isNullOrBlank()) {
            mode = SearchMode.Query
            search.onQuery(text)
        }
    }
    val startVoice = {
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH)
            .putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            .putExtra(RecognizerIntent.EXTRA_LANGUAGE, java.util.Locale.getDefault())
        try {
            voice.launch(intent)
        } catch (_: ActivityNotFoundException) {
            Toast.makeText(context, "这台设备没有可用的语音识别服务", Toast.LENGTH_SHORT).show()
        }
    }
    val openEntry = { entry: SearchEntry ->
        recent.add(entry.title)
        recentVersion++
        onOpenEntry(entry)
    }

    BackHandler {
        when (mode) {
            SearchMode.Query -> { search.clear(); mode = SearchMode.Home }
            SearchMode.Browse -> mode = SearchMode.Home
            SearchMode.Home -> onClose()
        }
    }
    DisposableEffect(Unit) {
        onDispose { search.clear(); browse.close() }
    }

    Box(Modifier.fillMaxSize()) {
        AnimatedContent(
            targetState = mode,
            transitionSpec = { fadeIn(tween(duration)) togetherWith fadeOut(tween(duration)) },
            label = "searchMode",
        ) { target ->
            when (target) {
                SearchMode.Home -> SearchHomePage(
                    recent = remember(recentVersion) { recent.list() },
                    hot = hot,
                    artwork = artwork,
                    onOpenQuery = {
                        // ①搜索框与②最佳匹配大卡位置重叠：保留旧结果时，用户连点搜索框的第二下
                        // 会落在大卡上直接进详情（真机复现）。从①进②一律开始新搜索，清空旧结果。
                        search.clear()
                        mode = SearchMode.Query
                    },
                    onVoice = startVoice,
                    onRecentSelect = { term -> mode = SearchMode.Query; search.onQuery(term) },
                    onClearRecent = { recent.clear(); recentVersion++ },
                    onCategory = { category -> browse.open(category); mode = SearchMode.Browse },
                    onHotSelect = { entry ->
                        recent.add(entry.title); recentVersion++
                        mode = SearchMode.Query; search.onQuery(entry.title)
                    },
                )
                SearchMode.Query -> SearchQueryPage(
                    repo = search,
                    onCancel = { search.clear(); mode = SearchMode.Home },
                    onVoice = startVoice,
                    onOpen = openEntry,
                    onSubmitQuery = {
                        // 键盘「搜索」键提交也计入最近搜索（此前只有点结果/热搜才记录）。
                        val term = search.query.trim()
                        if (term.isNotEmpty()) { recent.add(term); recentVersion++ }
                    },
                )
                SearchMode.Browse -> SearchBrowsePage(
                    repo = browse,
                    onBack = { mode = SearchMode.Home },
                    onOpen = openEntry,
                )
            }
        }
        if (mode != SearchMode.Query) {
            Box(Modifier.align(Alignment.BottomCenter).navigationBarsPadding().padding(bottom = 22.dp)) {
                navSlot()
            }
        }
    }
}