package com.kivo.cinema.ui.design.settings

import androidx.activity.compose.BackHandler
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 数据与资源子页路由（266 设计 §二）：根页 + 8 来源钻入页，子页内部屏态（不升级 SettingsNav 栈）。
 * 持 SourceRepository + 源列表态：LaunchedEffect IO 读 accounts.bin；每源开关真写回（setControl）后刷新。
 * 内层 BackHandler 先消费钻入返回；根页返回交 SettingsNav。隐藏源不列出（私密区管理）。
 */
@Composable
internal fun SettingsDataSub(store: SettingsStore, onOpenSources: () -> Unit, onBack: () -> Unit) {
    val context = LocalContext.current
    val repo = remember { SourceRepository(SourceStore(context)) }
    val scope = rememberCoroutineScope()
    var records by remember { mutableStateOf<List<SourceRecord>>(emptyList()) }
    LaunchedEffect(Unit) {
        records = withContext(Dispatchers.IO) { repo.read() }.getOrElse { emptyList() }
    }
    val onControl: (SourceRecord, SourceRepository.Control, Boolean) -> Unit = { record, control, on ->
        scope.launch {
            val result = withContext(Dispatchers.IO) {
                repo.setControl(record.id, record.revision, control, on)
            }
            result.onSuccess { records = it }
        }
    }
    var category by rememberSaveable { mutableStateOf<DataSourceCategory?>(null) }
    BackHandler(enabled = category != null) { category = null }
    val current = category
    if (current == null) {
        SettingsDataRoot(store, records, onOpenSources, onBack) { category = it }
    } else {
        DataSourcePage(current, records, onControl, onOpenSources) { category = null }
    }
}