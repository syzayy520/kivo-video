package com.kivo.cinema.playback

import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.mpv.MpvSession

/**
 * 274-HDR · **L2 = HDR→SDR 色调映射**的落地（只做 L2，不做真 HDR 输出）。
 *
 * ## 为什么需要它（真机实测，2026-11-15 04:2x）
 *
 * 片源是 HDR10（hevc Main10 / bt2020nc / **smpte2084(PQ)** / bt2020）时，
 * 现有 render API 路径（`vo=libmpv` + legacy RA）**没有 compute shader**
 * ⇒ libplacebo 关掉了 HDR 峰值计算
 * （`Disabling HDR peak computation (compute shaders=0, SSBO=1)`）
 * ⇒ PQ 码值被**直接当作 SDR 显示** ⇒ **画面明显偏暗、偏灰**。
 *
 * 三臂对照（同一 HDR10 素材、同一台 K40、同一时刻，`docs/evidence/p274-hdr/l2-ab/`）：
 *
 * | 臂 | 下发的 mpv 参数 | 设备截屏亮度（Rec.709 系数 / sRGB 编码值） |
 * | --- | --- | --- |
 * | A 基线 | 无 | mean **75.0** / max 159 / p95 144 |
 * | B L2 | `target-trc=bt.1886,target-prim=bt.709,target-peak=100,tone-mapping=bt.2390,target-colorspace-hint=yes` | mean **93.7** / max 181 / p95 169 |
 * | C 可视化 | `tone-mapping-visualize=yes` | mean 75.0（与基线逐像素相同 ⇒ 该可视化在无 tone-map 时不画曲线） |
 *
 * ⇒ **L2 有效**：同一个 PQ 画面，L2 之后亮部与整体亮度都回到正常 SDR 观感。
 *
 * ## 为什么只在 HDR 时才下发
 *
 * · SDR 片源**一个参数都不改** ⇒ 普通播放零回归风险（这是本实现的硬约束）。
 * · 这些是 **VO 级选项**（`target-*` / `tone-mapping`），改动会让 VO 重新初始化
 *   （内核日志里能看到 `Disabling HDR peak computation` 重新打印一次）⇒ 对 SDR 内容
 *   是"白折腾一次 VO 重建"，不值得。
 *
 * ## 已知边界（不夸大）
 *
 * · 这**不是** HDR 输出：面板仍是 SDR 路径，颜色被压回 bt.709/bt.1886。
 * · 真 HDR（G2）仍被架构闸门挡住（要 compute shader ⇒ 要换 `vo_gpu_next`），
 *   见 [274-hdr-rendering.md](../../../../docs/design/ui/replica/274-hdr-rendering.md) §10.0。
 * · 判定依据是内核回报的 `video-params`；**读不到就不动**（不许猜）。
 */
internal object MpvHdrToneMap {

    /** L2 参数组：SDR 目标 + BT.2390 色调映射 + 峰值按 SDR 参考白 100 nit。 */
    private val L2_OPTIONS = listOf(
        "target-trc" to "bt.1886",
        "target-prim" to "bt.709",
        "target-peak" to "100",
        "tone-mapping" to "bt.2390",
        "target-colorspace-hint" to "yes",
    )

    /**
     * 看内核回报的色彩参数，是 HDR 就下发 L2；不是就**什么都不做**。
     *
     * @return 一句话状态（写进诊断，也给上层显示用）；null = 没读到色彩参数。
     */
    fun applyIfHdr(native: MpvSession): String? {
        val params = runCatching { native.property("video-params") }.getOrNull()
        if (params.isNullOrBlank()) {
            NetDiag.log("[color] 读不到 video-params ⇒ 不改色彩参数")
            return null
        }
        val gamma = jsonString(params, "gamma")
        val primaries = jsonString(params, "primaries")
        val isHdr = gamma != null && HDR_GAMMAS.any { gamma.equals(it, ignoreCase = true) }
        if (!isHdr) {
            NetDiag.log("[color] 非 HDR（gamma=$gamma primaries=$primaries）⇒ 色彩参数不动")
            return "SDR（gamma=${gamma ?: "未知"}）"
        }
        val results = L2_OPTIONS.map { (k, v) ->
            val ok = native.setProperty(k, v)
            "$k=$v:${if (ok) "ok" else "失败"}"
        }
        NetDiag.log("[color] HDR 检出（gamma=$gamma primaries=$primaries）⇒ L2 tone-map 已下发：${results.joinToString(" ")}")
        return "HDR→SDR 色调映射（$gamma）"
    }

    /** mpv 报告的 HDR 传输函数。 */
    private val HDR_GAMMAS = listOf("pq", "hlg", "st2084", "bt.2020-pq", "bt.2020-hlg")

    /**
     * 从 mpv 的 `video-params`（一段 JSON 文本）里取一个字符串字段。
     *
     * 【为什么自己抠而不是用 JSON 库】`video-params` 的形状随 mpv 版本变，
     * 而且我们只要两个字段 ⇒ 轻量解析更稳，也避免为一个字段引入依赖。
     */
    private fun jsonString(json: String, key: String): String? {
        val marker = "\"$key\""
        val at = json.indexOf(marker)
        if (at < 0) return null
        val colon = json.indexOf(':', at + marker.length)
        if (colon < 0) return null
        val rest = json.substring(colon + 1).trimStart()
        if (!rest.startsWith("\"")) return null
        val end = rest.indexOf('"', 1)
        if (end < 0) return null
        return rest.substring(1, end)
    }
}
