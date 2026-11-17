package com.kivo.cinema.playback

/**
 * 指标 JSON 的通用转义与**片段渲染**。
 *
 * 从 [PlaybackMetrics] 拆出（原文件 312 行已超 300，且 345 又要加 `mpv_health`）。
 * 拆分口径：本文件只管「把已记录的数据变成 JSON 文本」，不持有任何状态；
 * 状态与生命周期留在 [PlaybackMetrics]。这样渲染改动不会碰采集逻辑。
 */

/** JSON 字符串转义（只处理必需字符，标签/属性值都是 ASCII 或中文）。 */
internal fun metricsJsonQuote(s: String): String {
    val sb = StringBuilder(s.length + 2)
    sb.append('"')
    for (c in s) {
        when (c) {
            '"' -> sb.append("\\\"")
            '\\' -> sb.append("\\\\")
            '\n' -> sb.append("\\n")
            '\r' -> sb.append("\\r")
            '\t' -> sb.append("\\t")
            else -> if (c < ' ') sb.append("\\u").append(String.format("%04x", c.code)) else sb.append(c)
        }
    }
    sb.append('"')
    return sb.toString()
}

/**
 * seek 块：单次明细 + 汇总分位数（275 §6.2「连续 20 次 seek 无旧请求覆盖新目标」）。
 *
 * @param seeks 本场会话的全部 seek 记录，调用方保证已按发生顺序追加。
 */
internal fun renderSeekBlock(seeks: List<PlaybackMetrics.SeekRec>): String {
    fun pct(xs: List<Long>, p: Double): String =
        if (xs.isEmpty()) "\"not_measured\"" else xs[((xs.size - 1) * p).toInt()].toString()

    val totals = seeks.map { it.totalMs }.sorted()
    // abs() 之后恒 >= 0，filter 保留是为了不改动既有口径（历史证据按此生成）。
    val errs = seeks.map { kotlin.math.abs(it.landingUs - it.requestedUs) / 1000 }
        .filter { it >= 0 }.sorted()

    val sb = StringBuilder()
    sb.append("  \"seeks\": {\n")
    sb.append("    \"count\": ").append(seeks.size).append(",\n")
    sb.append("    \"total_ms_p50\": ").append(pct(totals, 0.50)).append(",\n")
    sb.append("    \"total_ms_p95\": ").append(pct(totals, 0.95)).append(",\n")
    sb.append("    \"total_ms_max\": ").append(pct(totals, 1.0)).append(",\n")
    sb.append("    \"error_ms_p50\": ").append(pct(errs, 0.50)).append(",\n")
    sb.append("    \"error_ms_max\": ").append(pct(errs, 1.0)).append(",\n")
    sb.append("    \"failed\": ").append(seeks.count { it.error != null }).append(",\n")
    sb.append("    \"items\": [")
    sb.append(seeks.joinToString(", ") { r ->
        "{\"req_ms\":${r.requestedUs / 1000},\"land_ms\":${r.landingUs / 1000}," +
            "\"total_ms\":${r.totalMs}" +
            (if (r.error != null) ",\"error\":${metricsJsonQuote(r.error)}" else "") + "}"
    })
    sb.append("]\n  },\n")
    return sb.toString()
}
