package com.kivo.cinema.emby

import org.json.JSONArray

/**
 * 274：**从 Emby 的 `MediaSources` 里挑一个"播得动"的版本。**
 *
 * ## 为什么必须有它（真机实测）
 *
 * 同一个条目经常有多个版本。以 Emby `485`（紫罗兰 4K）为例：
 *
 * | MediaSource | 版本 | 码率 | 大小 | 我们能不能播 |
 * | --- | --- | --- | --- | --- |
 * | `aafdd622…` | **1080p** | **0.49 MB/s**（4.1 Mbps） | 4.1 GB | ✅ 能力 1.10–2.16 MB/s |
 * | `f8d2f0a0…` | 2160p | **6.51 MB/s**（54.6 Mbps） | 54 GB | ❌ 实测 1.53 fps |
 *
 * 而旧代码取的是 `MediaSources[0]` —— 恰好是 4K 那一个 ⇒
 * 用户播 4K 片源时直接崩到 1.53 fps、起播 34 秒（273 规格 §二十六）。
 *
 * **Emby 支持用 `MediaSourceId` 指定版本**（真机验证：加参数后
 * `Content-Range` 总长从 57,523,754,510 变成 4,316,184,874）⇒
 * 不必转码、不必改 UI，选对版本就能播。
 *
 * ## 为什么只影响播放，不影响展示
 *
 * 本类**只回答"用哪个 MediaSourceId 播"**。详情页展示的技术规格仍取
 * `MediaSources[0]`，用户看到的还是片源本来面目 —— 不动 UI。
 *
 * 【为什么不是 internal】`androidTest` 是独立模块，看不到 app 的 `internal`
 * ⇒ 想用 instrumentation 验证选源结果，必须是 public。这个类是纯函数式工具，公开无副作用。
 *
 * ## 阈值怎么定
 *
 * [MAX_PLAYABLE_BPS] = **12 Mbps（1.5 MB/s）**：真机单连接能力 1.10–2.16 MB/s，
 * 取中间偏保守值，留出抖动余量。低于此的版本才有机会流畅。
 *
 * 全都超标时**退回码率最小的那个** —— 它仍是最可能播得动的，
 * 并且由 [Pick.reason] 明确写出"全部超阈值"，不静默。
 */
object EmbySourcePicker {

    /** 单条流的码率上限（bps）。超过它就不指望能流畅。 */
    private const val MAX_PLAYABLE_BPS = 12_000_000L

    /**
     * 选中的版本。
     *
     * @param id `MediaSourceId`；null = 这个条目没给 MediaSources（保持旧行为，URL 不带参数）
     * @param bps 该版本的码率（bps）；0 = 未知
     * @param reason 为什么选它（写日志用，不许静默决策）
     */
    private data class Item(
        val id: String,
        val bps: Long,
        val sizeBytes: Long,
        val name: String,
    )

    data class Pick(val id: String?, val bps: Long, val sizeBytes: Long, val reason: String)

    /**
     * 挑播放用的版本。
     *
     * 规则（**先满足可播，再求最好**）：
     * 1. 只要码率 ≤ [MAX_PLAYABLE_BPS] 的，取其中**码率最高**的（画质最好）；
     * 2. 一个都没有 ⇒ 取**码率最低**的（最可能播得动）；
     * 3. 码率未知（0）⇒ 视为可播，但排在已知码率之后，避免把"未知"当"最优"；
     * 4. 只有一个版本 ⇒ 直接用它。
     */
    fun pick(sources: JSONArray?): Pick {
        if (sources == null || sources.length() == 0) {
            return Pick(null, 0L, 0L, "无 MediaSources（保持旧行为）")
        }
        val items = (0 until sources.length()).mapNotNull { i ->
            val o = sources.optJSONObject(i) ?: return@mapNotNull null
            val id = o.optString("Id").takeIf { it.isNotBlank() } ?: return@mapNotNull null
            val bps = o.optLong("Bitrate", 0L).coerceAtLeast(0L)
            val size = o.optLong("Size", 0L).coerceAtLeast(0L)
            val name = o.optString("Name").ifBlank { "?" }
            Item(id, bps, size, name)
        }
        if (items.isEmpty()) return Pick(null, 0L, 0L, "MediaSources 里没有可用 Id（保持旧行为）")
        if (items.size == 1) {
            val (id, bps, size, name) = items.first()
            return Pick(id, bps, size, "只有一个版本（$name，$bps bps）")
        }

        val playable = items.filter { it.bps in 1..MAX_PLAYABLE_BPS }
        if (playable.isNotEmpty()) {
            val best = playable.maxByOrNull { it.bps }!!
            return Pick(
                best.id, best.bps, best.sizeBytes,
                "可播版本里码率最高（${best.name}，${best.bps} bps ≤ $MAX_PLAYABLE_BPS）",
            )
        }

        // 全超阈值：退回最小 —— 但仍要说清楚（不许静默降级）
        val smallest = items.filter { it.bps > 0 }.minByOrNull { it.bps } ?: items.first()
        return Pick(
            smallest.id, smallest.bps, smallest.sizeBytes,
            "全部超阈值（最小仍是 ${smallest.name} / ${smallest.bps} bps > $MAX_PLAYABLE_BPS），" +
                "只能用最小的那个",
        )
    }
}
