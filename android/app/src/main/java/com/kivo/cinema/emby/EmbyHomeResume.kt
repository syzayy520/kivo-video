package com.kivo.cinema.emby

import android.content.Context
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import java.util.concurrent.Callable
import java.util.concurrent.Executors

/**
 * 首页「继续观看」的 Emby 侧取数。
 *
 * 单一职责：读已配置的可见 Emby 源 → 登录 → `Users/{uid}/Items/Resume` → [HomeEntry]。
 * 不做 UI、不持有状态；**失败一律静默降级为空**——首页不能因某个源不可达而报错或白屏。
 *
 * 两处关键性能点（真机测出来的，不是猜的）：
 * 1. **复用会话**：`refreshContinue` 随 onResume 会被多次触发，不复用就会反复 login（每次一次往返）。
 * 2. **多源并发**：串行时每源各付一次 login+resume，真机 3 个源累加 ≈2.4s；并发后取最慢那源。
 *
 * 阻塞调用（有网络往返）：必须在 `Dispatchers.IO` / 后台线程执行。
 */
object EmbyHomeResume {

    fun load(context: Context, limitPerSource: Int = 6): List<HomeEntry> {
        val records = runCatching {
            SourceRepository(SourceStore(context)).read().getOrElse { emptyList() }
        }.getOrElse { emptyList() }
            .filter { !it.hidden && !it.isSmb && it.aggregateEnabled }
        if (records.isEmpty()) return emptyList()

        val pool = Executors.newFixedThreadPool(minOf(records.size, MAX_PARALLEL))
        return try {
            records
                .map { rec -> pool.submit(Callable { fetchOne(rec, limitPerSource) }) }
                .flatMap { future -> runCatching { future.get() }.getOrElse { emptyList() } }
        } finally {
            pool.shutdown()
        }
    }

    private fun fetchOne(rec: SourceRecord, limitPerSource: Int): List<HomeEntry> =
        runCatching {
            // 优先复用已登录会话，避免每轮刷新都重新 login。
            val cached = EmbyResumeSession.session(rec.id)
            val client = cached?.first ?: EmbyClient(rec.address)
            val auth = cached?.second ?: client.login(rec.username, rec.password)
            if (cached == null) EmbyResumeSession.remember(rec.id, client, auth)
            EmbyHeroSource(client).resume(auth, limit = limitPerSource)
                .map { it.toHomeEntry(rec.id, rec.name) }
        }.getOrElse { emptyList() }

    private fun EmbyHeroItem.toHomeEntry(sourceId: String, sourceName: String) = HomeEntry(
        key = "emby:$sourceId:$id",
        title = name,
        sourceLabel = sourceName,
        addedSec = 0L,
        positionMs = resumeMs,
        durationMs = runtimeMs,
        kind = HomeEntry.Kind.Emby,
        posterUrl = heroUrl.ifBlank { null },
        embyItemId = id,
        embySourceId = sourceId,
        // Emby resume 自带"最近观看"排序，但没有时间戳字段 ⇒ 记 0，由聚合器稳定排在其后。
        lastPlayedMs = 0L,
    )

    /** 同时最多连 4 个源；再多对首屏没收益，只把源站并发撑高。 */
    private const val MAX_PARALLEL = 4
}
