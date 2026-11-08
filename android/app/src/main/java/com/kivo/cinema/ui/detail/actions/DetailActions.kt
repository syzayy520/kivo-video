package com.kivo.cinema.ui.detail.actions

import android.widget.Toast
import androidx.compose.runtime.*
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.media.marks.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

internal class DetailActions(
    val marks: FilmMarks,
    val enabled: Boolean,
    val error: String?,
    val toggle: (FilmMark) -> Unit,
    val retry: () -> Unit,
)

@Composable
internal fun rememberDetailActions(key: FilmKey): DetailActions {
    val context = LocalContext.current
    val store = remember { FilmMarksStore.get(context) }
    val snapshot by store.snapshots.collectAsState()
    val scope = rememberCoroutineScope()
    var saving by remember(key) { mutableStateOf(false) }
    var writeError by remember(key) { mutableStateOf<String?>(null) }
    val refresh: () -> Unit = {
        scope.launch { withContext(Dispatchers.IO) { store.refresh() } }
    }
    LaunchedEffect(store) { withContext(Dispatchers.IO) { store.refresh() } }
    return DetailActions(snapshot[key], snapshot.ready && !saving,
        writeError ?: snapshot.error, toggle = { mark ->
            if (snapshot.ready && !saving) {
                saving = true
                writeError = null
                val enabled = !snapshot[key].contains(mark)
                scope.launch {
                    try {
                        val result = withContext(Dispatchers.IO) { store.set(key, mark, enabled) }
                        result.onSuccess {
                            val label = when (mark) {
                                FilmMark.FAVORITE -> if (enabled) "已收藏" else "已取消收藏"
                                FilmMark.WATCHLIST -> if (enabled) "已添加到待看" else "已移出待看"
                                FilmMark.WATCHED -> if (enabled) "已标记为已观看" else "已取消已观看标记"
                            }
                            Toast.makeText(context, label, Toast.LENGTH_SHORT).show()
                        }.onFailure {
                            writeError = "保存失败：${it.message ?: "请检查设备存储空间后重试"}"
                        }
                    } finally { saving = false }
                }
            }
        }, retry = {
            writeError = null
            refresh()
        })
}
