package com.kivo.cinema.emby.aggregate

import android.content.Context
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyErrors
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.access.SourceEligibility
import com.kivo.cinema.sources.storage.SourceStore
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Semaphore
import kotlinx.coroutines.sync.withPermit
import kotlinx.coroutines.withContext

/**
 * 256 跨源连接会话：读源 → eligibility.search + 解锁过滤 → Semaphore(2) 并行登录 →
 * 已连接句柄 + 每源非空失败 reason。沿用 [com.kivo.cinema.sources.overview.SourceSummaries] 的限流先例。
 * 单一职责：把"哪些服务器接入了、哪些没接入为什么"变成主线程可观察态；不跑搜索（在 AggregateSearchModel）。
 */
class AggregateSession(private val scope: CoroutineScope) {
    data class Failure(val name: String, val reason: String)

    var connecting by mutableStateOf(false); private set
    var sources by mutableStateOf<List<AggregateSource>>(emptyList()); private set
    var failures by mutableStateOf<List<Failure>>(emptyList()); private set
    var error by mutableStateOf<String?>(null); private set
    private val slots = Semaphore(2)
    private var job: Job? = null

    fun connect(context: Context) {
        if (connecting) return
        connecting = true; error = null; failures = emptyList(); sources = emptyList()
        val app = context.applicationContext
        job = scope.launch {
            val records = withContext(Dispatchers.IO) { SourceRepository(SourceStore(app)).read() }
                .getOrElse {
                    error = "无法读取资源库，请返回资源库后重试"; connecting = false; return@launch
                }
            val eligible = SourceEligibility.search(records).filter {
                !it.isSmb && SourceEligibility.visible(it, SourceAccessSession.unlocked)
            }
            if (eligible.isEmpty()) {
                error = "没有可聚合搜索的 Emby 资源库。请先在资源库添加服务器，并在管理页开启「聚合搜索」。"
                connecting = false; return@launch
            }
            val outcomes = eligible.map { record ->
                async { slots.withPermit { withContext(Dispatchers.IO) { record to runCatching { login(record) } } } }
            }.awaitAll()
            sources = outcomes.mapNotNull { (_, r) -> r.getOrNull() }
            failures = outcomes.mapNotNull { (record, r) ->
                r.exceptionOrNull()?.let { Failure(record.name, EmbyErrors.reason(it)) }
            }
            if (sources.isEmpty())
                error = "全部资源库接入失败：" + (failures.firstOrNull()?.reason ?: "未知原因")
            connecting = false
        }
    }

    private fun login(record: SourceRecord): AggregateSource {
        val client = EmbyClient(record.address)
        return AggregateSource(record, client, client.login(record.username, record.password))
    }

    fun cancel() { job?.cancel(); connecting = false }
    fun close() { cancel(); sources = emptyList(); failures = emptyList(); error = null }
}