package com.kivo.cinema.emby

import android.content.Context
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore

/**
 * 首页「继续观看」的 Emby 侧取数。
 *
 * 单一职责：读已配置的可见 Emby 源 → 登录 → `Users/{uid}/Items/Resume` → [HomeEntry]。
 * 不做 UI、不持有状态；**失败一律静默降级为空**——首页不能因某个源不可达而报错或白屏。
 *
 * 阻塞调用（有网络往返）：必须在 `Dispatchers.IO` / 后台线程执行。
 * 只取 `aggregateEnabled`（参与聚合）且未隐藏的 Emby 源；SMB 源不在本对象处理。
 */
object EmbyHomeResume {

    fun load(context: Context, limitPerSource: Int = 6): List<HomeEntry> {
        val records = runCatching {
            SourceRepository(SourceStore(context)).read().getOrElse { emptyList() }
        }.getOrElse { emptyList() }
        return records
            .filter { !it.hidden && !it.isSmb && it.aggregateEnabled }
            .flatMap { rec ->
                runCatching {
                    val client = EmbyClient(rec.address)
                    val auth = client.login(rec.username, rec.password)
                    // 供海报组件复用会话，避免每张图重新登录。
                    EmbyResumeSession.remember(rec.id, client, auth)
                    EmbyHeroSource(client).resume(auth, limit = limitPerSource)
                        .map { it.toHomeEntry(rec.id, rec.name) }
                }.getOrElse { emptyList() }
            }
    }

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
}
