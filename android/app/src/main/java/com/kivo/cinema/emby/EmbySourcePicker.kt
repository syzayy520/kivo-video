package com.kivo.cinema.emby

import com.kivo.cinema.playback.Av1HardwareProbe
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
 * ## 301：为什么还要看编码格式，不能只看码率
 *
 * 301 实测（K40）：**4K AV1 10bit 只有 10.87 fps**，而它的码率（11.9 Mbps）
 * **低于** 12 Mbps 阈值 ⇒ 只按码率判断会把它选成"可播版本里画质最好的"，
 * 结果播不动。反过来 1080p AV1 软解 **60.44 fps** 完全可播 ⇒
 * 判据必须是 **codec × 分辨率 × 设备硬解能力**，不是码率一条线。
 *
 * 所以 `Item` 带上 `codec`/`height`，先用 [Av1HardwareProbe.isLikelyUnplayable]
 * 过一道"编码闸门"。**只有实测确认跑不动的组合才拦**（4K+ AV1 且无硬解），
 * 其余一律按原码率规则 —— 不猜、不过度降权。
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
        /** `MediaStreams` 里第一条 Video 的 `Codec`（"av1"/"hevc"/"h264"）；null = 没给。 */
        val codec: String?,
        /** 同上 Video 的 `Height`；0 = 没给。 */
        val height: Int,
    )

    data class Pick(val id: String?, val bps: Long, val sizeBytes: Long, val reason: String)

    /**
     * 挑播放用的版本。
     *
     * 规则（**先满足可播，再求最好**）：
     * 0. **编码闸门（301）**：4K+ 的 AV1 且本机无 AV1 硬解 ⇒ 直接判不可播
     *    （实测 10.87 fps）。其余组合不受影响 —— 1080p AV1 软解 60.44 fps 可播。
     * 1. 只要码率 ≤ [MAX_PLAYABLE_BPS] 的，取其中**码率最高**的（画质最好）；
     * 2. 一个都没有 ⇒ 取**码率最低**的（最可能播得动）；
     * 3. 码率未知（0）⇒ 视为可播，但排在已知码率之后，避免把"未知"当"最优"；
     * 4. 只有一个版本 ⇒ 直接用它。
     *
     * @param av1Hw 本机是否有 AV1 硬解；默认现探，测试可注入。
     */
    fun pick(
        sources: JSONArray?,
        av1Hw: Boolean = Av1HardwareProbe.hasHardwareAv1(),
    ): Pick {
        if (sources == null || sources.length() == 0) {
            return Pick(null, 0L, 0L, "无 MediaSources（保持旧行为）")
        }
        val items = (0 until sources.length()).mapNotNull { i ->
            val o = sources.optJSONObject(i) ?: return@mapNotNull null
            val id = o.optString("Id").takeIf { it.isNotBlank() } ?: return@mapNotNull null
            val bps = o.optLong("Bitrate", 0L).coerceAtLeast(0L)
            val size = o.optLong("Size", 0L).coerceAtLeast(0L)
            val name = o.optString("Name").ifBlank { "?" }
            // 每个 MediaSource 有自己的 MediaStreams；取第一条 Video 的 codec/height。
            val video = o.optJSONArray("MediaStreams")?.let { arr ->
                (0 until arr.length()).mapNotNull { arr.optJSONObject(it) }
                    .firstOrNull { it.optString("Type") == "Video" }
            }
            val codec = video?.optString("Codec")?.takeIf { it.isNotBlank() }
            val height = video?.optInt("Height", 0) ?: 0
            Item(id, bps, size, name, codec, height)
        }
        if (items.isEmpty()) return Pick(null, 0L, 0L, "MediaSources 里没有可用 Id（保持旧行为）")
        if (items.size == 1) {
            val only = items.first()
            val bad = Av1HardwareProbe.isLikelyUnplayable(only.codec, only.height, av1Hw)
            val why = if (bad) "；⚠️ 该版本是 4K+ AV1 且本机无 AV1 硬解，实测无法实时播放" else ""
            return Pick(
                only.id, only.bps, only.sizeBytes,
                "只有一个版本（${only.name}，${only.bps} bps）$why",
            )
        }

        // 301 · 编码闸门：只把「实测确认跑不动」的组合排除，其余保持原规则。
        val (codecOk, codecBad) = items.partition {
            !Av1HardwareProbe.isLikelyUnplayable(it.codec, it.height, av1Hw)
        }
        if (codecOk.isEmpty()) {
            // 全部都是 4K+ AV1 且无硬解 ⇒ 只能选码率最小的，并把原因写清（不许静默）。
            val smallest = codecBad.filter { it.bps > 0 }.minByOrNull { it.bps } ?: codecBad.first()
            return Pick(
                smallest.id, smallest.bps, smallest.sizeBytes,
                "所有版本都是 4K+ AV1 且本机无 AV1 硬解（实测 10.87 fps）；" +
                    "只能选码率最小的 ${smallest.name}（${smallest.bps} bps）",
            )
        }

        val playable = codecOk.filter { it.bps in 1..MAX_PLAYABLE_BPS }
        if (playable.isNotEmpty()) {
            val best = playable.maxByOrNull { it.bps }!!
            val skipped = if (codecBad.isNotEmpty()) "；已跳过 ${codecBad.size} 个 4K+ AV1（无硬解）" else ""
            return Pick(
                best.id, best.bps, best.sizeBytes,
                "可播版本里码率最高（${best.name}，${best.bps} bps ≤ $MAX_PLAYABLE_BPS）$skipped",
            )
        }

        // 全超阈值：退回最小 —— 但仍要说清楚（不许静默降级）
        val smallest = codecOk.filter { it.bps > 0 }.minByOrNull { it.bps } ?: codecOk.first()
        return Pick(
            smallest.id, smallest.bps, smallest.sizeBytes,
            "全部超阈值（最小仍是 ${smallest.name} / ${smallest.bps} bps > $MAX_PLAYABLE_BPS），" +
                "只能用最小的那个",
        )
    }
}
