package com.kivo.cinema.sources.overview

import android.os.SystemClock
import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.sources.SourceRecord
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Semaphore
import kotlinx.coroutines.sync.withPermit
import kotlinx.coroutines.withContext

/** Memory-only user-scoped totals. Visible cards request them; at most two servers run at once. */
class SourceSummaries(private val scope: CoroutineScope, private val current: () -> List<SourceRecord>) {
    data class Summary(val revision: Long, val loading: Boolean = true,
        val counts: EmbyClient.Counts? = null, val reason: String? = null, val at: Long = 0,
        val online: Boolean? = null,
        // 海报墙：最近影片（已过滤无图条目）+ 拉图 token。仅内存，不持久化、不进日志。
        val posters: List<EmbyItem> = emptyList(), val imageToken: String? = null)
    var values by mutableStateOf<Map<String, Summary>>(emptyMap()); private set
    private val slots = Semaphore(2)

    private data class Fetch(val result: Result<EmbyClient.Counts>, val posters: List<EmbyItem>,
        val token: String?, val online: Boolean)

    fun request(record: SourceRecord, force: Boolean = false) {
        if (!com.kivo.cinema.sources.access.SourceEligibility.visible(record,
                com.kivo.cinema.sources.access.SourceAccessSession.unlocked)) return
        val old = values[record.id]
        if (old?.revision == record.revision && (old.loading ||
                (!force && SystemClock.elapsedRealtime() - old.at < 300_000))) return
        values = values + (record.id to Summary(record.revision, online = old?.takeIf {
            it.revision == record.revision }?.online))
        scope.launch {
            val fetch = slots.withPermit {
                withContext(Dispatchers.IO) {
                    if (!com.kivo.cinema.sources.access.SourceEligibility.visible(record,
                            com.kivo.cinema.sources.access.SourceAccessSession.unlocked))
                        return@withContext Fetch(Result.failure<EmbyClient.Counts>(IllegalStateException()),
                            emptyList(), null, false)
                    val client = EmbyClient(record.address)
                    // 一次 login 供 counts 与海报共用；海报失败不拖累 counts/online 判定。
                    val auth = runCatching { client.login(record.username, record.password) }
                    val counts = auth.mapCatching { client.counts(it) }
                    val posters = auth.getOrNull()?.let { a ->
                        runCatching { client.items(a, limit = 6).items.filter { it.posterUrl.isNotBlank() } }
                            .onFailure { Log.w("SourceSummaries", "海报读取失败 ${record.address}: ${it.message}") }
                            .getOrDefault(emptyList())
                    } ?: emptyList()
                    val failure = counts.exceptionOrNull() as? EmbyClient.HttpFailure
                    Fetch(counts, posters, auth.getOrNull()?.token,
                        counts.isSuccess || failure?.code in listOf(401, 403) || client.isReachable())
                }
            }
            if (current().none { it.id == record.id && it.revision == record.revision }) return@launch
            val reason = fetch.result.exceptionOrNull()?.let {
                when (it) {
                    is EmbyClient.HttpFailure -> when (it.code) {
                        401, 403 -> "数量读取受限，请检查账号权限"
                        else -> "服务器暂未提供数量"
                    }
                    is java.net.SocketTimeoutException -> "数量读取超时"
                    else -> "数量暂不可用"
                }
            }
            values = values + (record.id to Summary(record.revision, false,
                fetch.result.getOrNull(), reason, SystemClock.elapsedRealtime(), fetch.online,
                fetch.posters, fetch.token))
        }
    }
}
