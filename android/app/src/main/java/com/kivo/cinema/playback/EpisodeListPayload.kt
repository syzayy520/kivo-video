package com.kivo.cinema.playback

import org.json.JSONArray
import org.json.JSONObject

/**
 * 317 · **剧集列表的 intent 载荷**（详情页 → 播放页），支持**多季**。
 *
 * ## 为什么走 intent 而不是播放页去查
 * 详情页（`EmbyResource`）本来就已经拉到了剧集；播放页再查一次是**重复请求**，
 * 而且要额外依赖服务器地址/鉴权。直接把列表打包进 intent：
 * · 零网络、零鉴权依赖；
 * · 数据与详情页**必然一致**；
 * · 换集/切季都不需要再查。
 *
 * ## 结构（多季）
 * ```
 * [ { i: 季序号, t: 季名, e: [ { i: 集号, l: 完整标签, n: 集名, u: 播放URL, t: 缩略图URL } ] } ]
 * ```
 * 季名取 Emby 的 `Season.Name`（真机上是「清潭国际高中」「清潭国际高中 2」这种），
 * **不是**我们拼的"第N季" —— 因为特辑/特别版的季名无法用序号表达
 * （真机实例：某剧 2 正季 + 1 个"皇家训练营"特别版，序号表达不了）。
 *
 * ## 体积
 * 一集约 150 字节（URL 占大头）。实测 24 集 × 10 季 ≈ 36 KB，远低于 Binder 上限（1 MB）。
 * [MAX_SEASONS] / [MAX_EPISODES_PER_SEASON] 兜底截断，防 IntentTooLarge。
 *
 * ## 当前集怎么标记
 * **不存"当前是哪集"，让调用方用 URL 比对**（[locate]）。
 * 理由：换集时 `switchEpisodeImmediately` 会改 `intent` 的 `kivo.playurl`，
 * 只要 current 由 URL 推导，换完自动就对，不需要额外维护状态。
 */
internal object EpisodeListPayload {

    /** intent key。 */
    const val KEY = "kivo.episodeList"

    private const val MAX_SEASONS = 30
    private const val MAX_EPISODES_PER_SEASON = 500

    /** 一集。 */
    data class Entry(
        val index: Int,
        val label: String,        // 完整标签（连播提示用，如 "S01E01 贫贱"）
        val name: String,         // 集名（卡片标题）
        val url: String,
        val thumbUrl: String = "",
    )

    /** 一季：季序号、季名、该季全部集。 */
    data class SeasonBlock(val index: Int, val title: String, val entries: List<Entry>)

    /** 打包（详情页调用）。只保留有集的季 —— 空季在播放器里没有意义。 */
    fun encode(blocks: List<SeasonBlock>): String {
        val arr = JSONArray()
        blocks.filter { it.entries.isNotEmpty() }.take(MAX_SEASONS).forEach { b ->
            val eps = JSONArray()
            b.entries.take(MAX_EPISODES_PER_SEASON).forEach { e ->
                eps.put(
                    JSONObject()
                        .put("i", e.index)
                        .put("l", e.label)
                        .put("n", e.name)
                        .put("u", e.url)
                        .put("t", e.thumbUrl),
                )
            }
            arr.put(JSONObject().put("i", b.index).put("t", b.title).put("e", eps))
        }
        return arr.toString()
    }

    /** 解包（播放页调用）。格式错误一律返回空表 —— 面板会退回"仅一集"的说明行。 */
    fun decode(raw: String?): List<SeasonBlock> {
        if (raw.isNullOrBlank()) return emptyList()
        return runCatching {
            val arr = JSONArray(raw)
            (0 until arr.length()).mapNotNull { i ->
                val o = arr.optJSONObject(i) ?: return@mapNotNull null
                val eps = o.optJSONArray("e") ?: return@mapNotNull null
                val entries = (0 until eps.length()).mapNotNull { j ->
                    val e = eps.optJSONObject(j) ?: return@mapNotNull null
                    val url = e.optString("u")
                    if (url.isBlank()) return@mapNotNull null
                    Entry(e.optInt("i"), e.optString("l"), e.optString("n"), url, e.optString("t"))
                }
                if (entries.isEmpty()) return@mapNotNull null
                SeasonBlock(o.optInt("i"), o.optString("t"), entries)
            }.sortedBy { it.index }
        }.getOrDefault(emptyList())
    }

    /** 按播放 URL 定位到「第几季、第几集」；找不到返回 (0, -1)。 */
    fun locate(blocks: List<SeasonBlock>, url: String?): Pair<Int, Int> {
        if (url.isNullOrBlank()) return 0 to -1
        blocks.forEachIndexed { si, b ->
            val ei = b.entries.indexOfFirst { it.url == url }
            if (ei >= 0) return si to ei
        }
        return 0 to -1
    }
}
