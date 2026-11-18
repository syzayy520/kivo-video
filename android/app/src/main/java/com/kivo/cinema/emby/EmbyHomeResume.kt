package com.kivo.cinema.emby

import android.content.Context
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import java.util.concurrent.Callable
import java.util.concurrent.ExecutorCompletionService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

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

    /**
     * @param onPartial 每有**一个源**返回就回调一次（已合并的部分结果），用于首页先到先显示；
     *        传 null 则只在全部结束时返回一次。回调在后台线程执行，调用方自行切主线程。
     */
    fun load(
        context: Context,
        limitPerSource: Int = 6,
        onPartial: ((List<HomeEntry>) -> Unit)? = null,
    ): List<HomeEntry> {
        val records = runCatching {
            SourceRepository(SourceStore(context)).read().getOrElse { emptyList() }
        }.getOrElse { emptyList() }
            .filter { !it.hidden && !it.isSmb && it.aggregateEnabled }
        if (records.isEmpty()) return emptyList()

        com.kivo.cinema.net.NetDiag.log("[home] emby 源数=${records.size} 地址=${records.joinToString { it.address }}")
        val pool = Executors.newFixedThreadPool(minOf(records.size, MAX_PARALLEL))
        val ecs = ExecutorCompletionService<Pair<String, List<HomeEntry>>>(pool)
        records.forEach { rec -> ecs.submit(Callable { fetchOne(rec, limitPerSource) }) }

        val out = mutableListOf<HomeEntry>()
        var okSources = 0
        val deadline = System.currentTimeMillis() + FETCH_TIMEOUT_MS
        try {
            // 先到先显示：**共享一个总预算**，而不是每源串行各付一次超时。
            // 坏源（真机 hohai login 卡死）只占住自己的线程，不影响已到源的渲染。
            for (i in records.indices) {
                val remain = deadline - System.currentTimeMillis()
                if (remain <= 0) {
                    com.kivo.cinema.net.NetDiag.log("[home] emby 预算耗尽，剩 ${records.size - i} 源未回")
                    break
                }
                val done = ecs.poll(remain, TimeUnit.MILLISECONDS) ?: break
                val res = runCatching { done.get() }.getOrNull()
                val name = res?.first ?: "?"
                val got = res?.second.orEmpty()
                com.kivo.cinema.net.NetDiag.log("[home] 源完成 $name n=${got.size}")
                if (got.isEmpty()) continue
                okSources++
                out += got
                val snapshot = out.toList()
                runCatching { onPartial?.invoke(snapshot) }
            }
            com.kivo.cinema.net.NetDiag.log("[home] emby 完成 成功源=$okSources/${records.size} 条=${out.size}")
        } finally {
            // 不等待卡住的线程：shutdownNow 让坏源尽快让路，主流程立即返回已有结果。
            pool.shutdownNow()
        }
        return out
    }

    /** 返回 (源名, 条目)：源名只为诊断定位坏源，不影响业务。 */
    private fun fetchOne(rec: SourceRecord, limitPerSource: Int): Pair<String, List<HomeEntry>> =
        rec.name to runCatching {
            // 优先复用已登录会话（D367：登录一次真机 1.2s，不能每轮都付）。
            val cached = EmbyAuthCache.get(rec)
            val client = cached?.first ?: EmbyClient(rec.address)
            val auth = cached?.second ?: client.login(rec.username, rec.password)
            if (cached == null) EmbyAuthCache.remember(rec, client, auth)
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

    /**
     * 单源预算：login + Resume 真机正常 0.6–1.5s；给 6s 是"慢但能成"的上限，
     * 超过即视为不可达，直接降级——首页继续观看绝不能为等一个坏源而整行不显示。
     */
    private const val FETCH_TIMEOUT_MS = 6_000L
}
