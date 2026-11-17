package com.kivo.cinema.ui.detail

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.tech.AudioTrackInfo
import com.kivo.cinema.library.tech.HdrKind
import com.kivo.cinema.library.tech.MediaTechInfo
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 详情页「技术信息」卡（W1）——把 `TechBadges` 的胶囊摘要展开成逐行明细。
 *
 * ## 冻结契约
 * `DetailTechRows(info: MediaTechInfo?)`；行序固定为 编码 → 位深 → HDR → 码率 → 音轨 → 色彩空间，
 * 每行只在对应字段「确实拿到值」时出现，`info == null` 或一行都判不出时**整卡不渲染**
 * （连 12dp 顶部外距一起消失，不在页面上留空洞）。
 *
 * ## 与 `TechBadges` 的口径一致性（同一份 D1 模型，不允许两处给不同结论）
 * | 维度 | 徽章（`TechBadges`） | 本卡 | 差异原因 |
 * | --- | --- | --- | --- |
 * | 位深 | 只画 10bit | 8/10 都画 | 明细卡是「查得到就列」，`null` 才是缺失 |
 * | HDR | 只画 HDR10/HLG/DV | 同左 | `None` = 确认 SDR、`Unknown` = 没判出，两者都不画 HDR 行 |
 * | 码率 | `kbps < 1000` 用 kbps | 同左 | 复用同一换算，避免两处单位不一致 |
 * | 音轨 | 只取第一条 | 全部拼接 | 多语言轨是明细卡要回答的问题 |
 *
 * ## 色彩空间为什么能推、SDR 为什么不推
 * `MediaTechInfo` 没有独立的原色/矩阵字段（P12-E1 实测：`color-format` 八/十 bit 同值，
 * 平台不下发内容色度）。但 HDR 的三种传递函数在原色上是**标准约束**的：
 * HDR10 / Dolby Vision 走 `BT.2020 + PQ`，HLG 走 `BT.2020 + HLG`（ITU-R BT.2100）。
 * 所以仅当 `hdr ∈ {HDR10, HLG, DolbyVision}` 时画 `BT.2020`；
 * `None`（确认 SDR）只知传递函数、不知原色是 601 还是 709 ⇒ **不画**（不猜、不写「未知」）。
 *
 * ## 视觉参数（13dp 屏边由调用方提供）
 * 白卡 canvasFill · 0.5dp hairline · 圆角 16 · 内 14dp；标题 16sp Bold，
 * label 13sp `textSub` 定宽 60dp，value 13sp `textMain` 单行省略；行间距 8dp。
 * 卡高 = 28（上下内距）+ 22（标题）+ 行数 × (8 + 17)。5 行 ⇒ 175dp，6 行 ⇒ 200dp。
 *
 * @param info 探测结果；`null` 表示未探测完成或探测失败，此时不画任何东西。
 */
@Composable
fun DetailTechRows(info: MediaTechInfo?) {
    if (info == null) return
    val rows = techRows(info)
    if (rows.isEmpty()) return
    val shape = RoundedCornerShape(CardCorner)
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = GapBefore)
            .clip(shape)
            .background(DesignTokens.canvasFill)
            .border(BorderWidth, DesignTokens.hairline, shape)
            .padding(CardPad),
    ) {
        Text(
            "技术信息",
            fontSize = 16.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        rows.forEach { row ->
            Spacer(Modifier.height(RowGap))
            Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                Text(
                    row.label,
                    modifier = Modifier.width(LabelWidth),
                    fontSize = 13.sp,
                    color = DesignTokens.textSub,
                    maxLines = 1,
                )
                Text(
                    row.value,
                    modifier = Modifier.weight(1f),
                    fontSize = 13.sp,
                    color = DesignTokens.textMain,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
    }
}

private data class TechRow(val label: String, val value: String)

/** 行序即展示顺序；任何一环判不出值就不产生该行。 */
private fun techRows(info: MediaTechInfo): List<TechRow> = buildList {
    info.videoCodec?.takeIf { it.isNotBlank() }?.let { add(TechRow("编码", it)) }
    info.bitDepth?.let { add(TechRow("位深", "${it}-bit")) }
    hdrValue(info.hdr)?.let { add(TechRow("HDR", it)) }
    bitrateValue(info.bitrateKbps)?.let { add(TechRow("码率", it)) }
    audioValue(info.audio)?.let { add(TechRow("音轨", it)) }
    colorSpaceValue(info.hdr)?.let { add(TechRow("色彩空间", it)) }
}

private fun hdrValue(hdr: HdrKind): String? = when (hdr) {
    HdrKind.HDR10 -> "HDR10"
    HdrKind.HLG -> "HLG"
    HdrKind.DolbyVision -> "Dolby Vision"
    HdrKind.None, HdrKind.Unknown -> null
}

/** 与 `TechBadges.bitrateLabel` 同一换算：<1000 用 kbps，否则一位小数的 Mbps。 */
private fun bitrateValue(kbps: Int?): String? {
    if (kbps == null || kbps <= 0) return null
    return if (kbps < 1000) "$kbps kbps" else "%.1f Mbps".format(kbps / 1000f)
}

/** 多轨用 ` / ` 拼接；声道缺失只写编码（不伪造 `0ch`），语言缺失或 `und` 不写。 */
private fun audioValue(tracks: List<AudioTrackInfo>): String? {
    if (tracks.isEmpty()) return null
    val joined = tracks.joinToString(" / ") { track ->
        val layout = channelLayout(track.channels)
        val head = if (layout == null) track.codec else "${track.codec} $layout"
        val lang = track.language?.takeIf { it.isNotBlank() && it != "und" }
        if (lang == null) head else "$head · $lang"
    }
    return joined.takeIf { it.isNotBlank() }
}

private fun channelLayout(channels: Int): String? = when (channels) {
    1 -> "1.0"
    2 -> "2.0"
    6 -> "5.1"
    8 -> "7.1"
    else -> if (channels > 0) "${channels}ch" else null
}

/** 仅确认 HDR 时给 `BT.2020`；SDR 的原色（601/709）平台不下发，故不画。 */
private fun colorSpaceValue(hdr: HdrKind): String? = when (hdr) {
    HdrKind.HDR10, HdrKind.HLG, HdrKind.DolbyVision -> "BT.2020"
    HdrKind.None, HdrKind.Unknown -> null
}

private val CardCorner = 16.dp
private val CardPad = 14.dp
private val RowGap = 8.dp
private val GapBefore = 12.dp
private val LabelWidth = 60.dp
private val BorderWidth = 0.5.dp

// ───────────────────────────── Preview ─────────────────────────────

private fun previewInfo(
    codec: String?, depth: Int?, kbps: Int?, hdr: HdrKind, audio: List<AudioTrackInfo>,
) = MediaTechInfo(
    width = 3840, height = 2160, videoCodec = codec, bitDepth = depth,
    bitrateKbps = kbps, hdr = hdr, audio = audio,
)

@Composable
private fun previewCanvas(content: @Composable () -> Unit) {
    Column(Modifier.background(DesignTokens.pageBackground).padding(DesignTokens.screenPad.dp)) {
        content()
    }
}

/** 满态：六行全给（含 12dp 顶部外距，卡体 200dp）。 */
@Preview(widthDp = 360, heightDp = 234)
@Composable
private fun DetailTechRowsFullPreview() {
    previewCanvas {
        DetailTechRows(
            previewInfo(
                codec = "HEVC", depth = 10, kbps = 78_200, hdr = HdrKind.HDR10,
                audio = listOf(AudioTrackInfo("EAC3", 6, "en"), AudioTrackInfo("AAC", 2, "zh")),
            ),
        )
    }
}

/** 空态：`info = null` ⇒ 整卡不渲染，只剩 13dp 屏边。 */
@Preview(widthDp = 360, heightDp = 40)
@Composable
private fun DetailTechRowsEmptyPreview() {
    previewCanvas { DetailTechRows(info = null) }
}