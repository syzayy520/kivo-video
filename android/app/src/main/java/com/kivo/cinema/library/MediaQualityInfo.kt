package com.kivo.cinema.library

/**
 * 从媒体文件名解析「画质档位」，供详情页资源卡显示，纯字符串、零 IO/网络。
 *
 * 【为什么需要】NAS(SMB) 片源拿分辨率要读远端文件头，代价高且慢（SmbDetail 旧注释因此把
 * quality/resolution 写死成空串）。但 NAS 文件名几乎都带 `1080p / 2160p / HDR / Dolby.Vision`
 * 等发布标签（MediaTitleParser 的 TAGS 正是在剔除它们）——直接从名字读是零成本且诚实的：
 * 读得到就显示，读不到就返回空、由 UI 不渲染胶囊，绝不伪造「4K」。
 *
 * 【与 MediaTitleParser 的分工】那个把标签从**片名**里剔掉（得到干净标题）；
 * 这个把标签**留下**当画质事实（得到胶囊/主标题）。同一个文件名的两种用法，互不重复解析逻辑。
 *
 * 【保守判据】
 * - 分辨率取 `2160p/4k/UHD → 4K`、`1080p → 1080P`、`720p → 720P`，认不出返回空。
 *   别名 `x264/h264` 之类**不**当分辨率（那是编码，不是画质档）。
 * - 动态范围：`Dolby Vision/DV → DOLBY VISION`、`HDR10+ → HDR10+`、`HDR10/HDR → HDR10`，
 *   无 HDR 标签 → `SDR`（作为卡片主标题，与 REX 一致：HDR 必须显式标注才生效，未标注即 SDR）。
 */
object MediaQualityInfo {

    data class Quality(
        /** 分辨率档（4K / 1080P / 720P）；认不出为空。 */
        val resolution: String,
        /** 动态范围档（DOLBY VISION / HDR10+ / HDR10）；普通片为空。 */
        val dynamicRange: String,
        /** HDR / 10bit 命中（用于主标题「HDR10 · 1080P」这类组合，任一在即为真）。 */
        val hasHdr: Boolean,
    )

    /** 展平成分隔符统一的匹配串：`. _ -` → 空格，便于按 token 边界匹配。 */
    fun from(fileName: String): Quality {
        val flat = fileName.replace('.', ' ').replace('_', ' ').replace('-', ' ').uppercase()
        val words = Regex("""\s+""").replace(flat, " ").trim()
        val dr = dynamicRangeOf(words)
        return Quality(
            resolution = resolutionOf(words),
            dynamicRange = dr,
            // hasHdr 由 dr 推导，共用同一套判定，绝不出现「档位说 HDR10、标志却说不是」的分裂。
            hasHdr = dr != "SDR",
        )
    }

    private fun resolutionOf(w: String): String = when {
        hasToken(w, "2160P") || hasToken(w, "4K") || hasToken(w, "UHD") -> "4K"
        hasToken(w, "1080P") -> "1080P"
        hasToken(w, "720P") -> "720P"
        else -> ""
    }

    private fun dynamicRangeOf(w: String): String = when {
        w.contains("DOLBY VISION") || hasToken(w, "DOVI") || hasToken(w, "DV") -> "DOLBY VISION"
        hasToken(w, "HDR10+") || hasToken(w, "HDR10P") || w.contains("HDR10PLUS") -> "HDR10+"
        hasToken(w, "HDR10") || hasToken(w, "HDR") -> "HDR10"
        // 无 HDR 标记即标准动态范围（REX 卡主标题同样显示 SDR）。
        // HDR 必须显式标注才会被当 HDR 处理，故「未标注 → SDR」是准确描述，不是伪造。
        else -> "SDR"
    }

    /** 按空格 token 全等匹配，避免 `4K` 误命中 `UHD4KX`、`HDR10` 误命中 `HDR10+`。 */
    private fun hasToken(w: String, token: String): Boolean =
        w.split(' ').any { it == token }
}
