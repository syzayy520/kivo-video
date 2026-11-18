package com.kivo.cinema.library

/**
 * 首页「继续观看」的跨源条目（本地文件 / SMB 网络共享 / Emby）。
 *
 * 单一职责：只描述"首页要展示的一条内容"，**不含 IO、不含 UI，也不依赖 UI 层的格式化函数**。
 * 由 [HomeFeedAggregator] 产出，`ContinueRow` 消费；时间/来源的文案（"3 天前 · Emby"）
 * 由 UI 层用 `relDays`/`sourceLabel` 现拼，数据层只给原始值。
 *
 * 两种载体二选一：
 * - 本地 / SMB：[mediaItem] 非空 —— 抽帧海报与播放都靠它；
 * - Emby：[embyItemId] + [embySourceId] 非空，海报用 [posterUrl]（服务器图，局域网可达）。
 */
data class HomeEntry(
    /** 统一续播键：本地绝对路径，或 `emby:<sourceId>:<itemId>`。聚合去重按它。 */
    val key: String,
    val title: String,
    /** 来源短标签，UI 直接显示（"本地" / 共享名 / "Emby"）。 */
    val sourceLabel: String,
    /** 入库秒（本地 DATE_ADDED / Emby DateCreated）；0 = 未知，UI 不显示"几天前"。 */
    val addedSec: Long,
    val positionMs: Long,
    val durationMs: Long,
    val kind: Kind,
    val mediaItem: MediaItem? = null,
    val posterUrl: String? = null,
    /** 库内封面（共享内路径）；非空时优先于 [posterUrl]。读图需配合 [key] 解析出的 sourceId/share。 */
    val smbPosterPath: String? = null,
    /** 发行年（TMDB 兜底匹配用）；0/null = 未知。 */
    val year: Int? = null,
    /** TMDB 兜底用的英文名（中文名搜不到时再试它）；null = 无。 */
    val searchAlias: String? = null,
    val embyItemId: String? = null,
    val embySourceId: String? = null,
    /** 最近一次观看时刻（毫秒）；聚合排序用。0 = 未知，排在后面。 */
    val lastPlayedMs: Long = 0L,
) {
    /** 0..1 的续播进度；时长未知时给 0（UI 不画进度条）。 */
    val progress: Float
        get() = if (durationMs > 0) (positionMs.toFloat() / durationMs).coerceIn(0f, 1f) else 0f

    enum class Kind { Local, Smb, Emby }
}
