package com.kivo.cinema.ui.detail

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.tech.AudioTrackInfo
import com.kivo.cinema.library.tech.HdrKind
import com.kivo.cinema.library.tech.MediaTechInfo
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 详情页「技术徽章」条（D2）——kivo 原创项，竞品 rex 没有（`docs/design/originality-matrix.md` §1）。
 *
 * ## 为什么原创
 * 把硬解/10bit/HDR 这些内核能力摆到用户面前，是「kivo 比 rex 好在哪」的同屏可举证差异；
 * 形态刻意与评分徽章（`RatingBadges`）拉开：评分是「色块 + 数值」，这里是「中性胶囊 + 能力高亮胶囊」。
 *
 * ## 视觉参数（◐ 推断，非帧实测）
 * 胶囊高 24dp / 圆角 12dp / 间距 6dp / 文字 11sp —— 按详情页既有的 13dp 屏边与 15sp 元信息行反推，
 * 目标是「不抢主播放钮的视觉层级」。若 D4 真机对比后需要改，只改本文件顶部常量。
 *
 * ## 与字面规则的三处收紧（显式偏差，均有原因）
 * 1. **尺寸缺失（任一维 ≤0）不画分辨率徽章**：D1 模型用 0 表示「拿不到」，此处的 HD 会变成假信息；
 *    只缺一边时短边必然算成 0 ⇒ 判 HD，同样是假信息，故收紧为任一维缺失即不画。
 * 2. **channels ≤0 时音轨只显示编码**：`0ch` 是伪造声道数（`KEY_CHANNEL_COUNT` 缺失）。
 * 3. **码率 ≤0 视为缺失**：与 D1 的 `bitrateKbps = null` 口径一致，避免 `0.0 Mbps`。
 *
 * ## 未覆盖
 * · 深色态：`fg` 由调用方传入，底色仍取 `DesignTokens` 浅色语义令牌（与详情页当前实现一致）。
 * · 多音轨：只取第一条（任务口径）；多语言轨的产品化留给后续卡片。
 *
 * @param info 探测结果；`null`（未探测/无视频轨）时整行不渲染，不画占位。
 * @param fg 中性胶囊的文字色；能力型胶囊固定用 `accentGreen`，不受此参数影响。
 */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun TechBadges(info: MediaTechInfo?, fg: Color = DesignTokens.textSub) {
    if (info == null) return
    val badges = techBadgeSpecs(info)
    if (badges.isEmpty()) return
    FlowRow(
        horizontalArrangement = Arrangement.spacedBy(BadgeGap),
        verticalArrangement = Arrangement.spacedBy(BadgeGap),
    ) {
        badges.forEach { badge -> TechPill(badge, fg) }
    }
}

/** 一枚徽章：`strong` = 能力型（4K / 10-bit / HDR10），用绿底绿字高亮。 */
private data class TechBadgeSpec(val text: String, val strong: Boolean)

/** 顺序即展示顺序：分辨率 → 编码 → 位深 → HDR → 码率 → 音轨。 */
private fun techBadgeSpecs(info: MediaTechInfo): List<TechBadgeSpec> = buildList {
    resolutionLabel(info)?.let { add(TechBadgeSpec(it, strong = it == "4K")) }
    info.videoCodec?.takeIf { it.isNotBlank() }?.let { add(TechBadgeSpec(it, strong = false)) }
    if (info.bitDepth == 10) add(TechBadgeSpec("10-bit", strong = true))
    hdrLabel(info.hdr)?.let { add(TechBadgeSpec(it, strong = it == "HDR10")) }
    bitrateLabel(info.bitrateKbps)?.let { add(TechBadgeSpec(it, strong = false)) }
    info.audio.firstOrNull()?.let { add(TechBadgeSpec(audioLabel(it), strong = false)) }
}

/**
 * 分辨率徽章按**短边**判定（4K UHD 的定义是短边 2160，竖屏 1080×2400 的短边仍是 1080）。
 * 旧实现用 `height >= 2160`，把 1080×2400 竖屏片源判成 4K——长边论在竖屏素材上必然误判。
 * 短边门槛与消费级档位一一对应：2160(4K UHD) / 1440(2K QHD) / 1080(FHD)，其余 HD。
 */
private fun resolutionLabel(info: MediaTechInfo): String? {
    if (info.width <= 0 || info.height <= 0) return null
    val shortSide = minOf(info.width, info.height)
    return when {
        shortSide >= 2160 -> "4K"
        shortSide >= 1440 -> "2K"
        shortSide >= 1080 -> "1080P"
        else -> "HD"
    }
}

private fun hdrLabel(hdr: HdrKind): String? = when (hdr) {
    HdrKind.HDR10 -> "HDR10"
    HdrKind.HLG -> "HLG"
    HdrKind.DolbyVision -> "DOLBY VISION"
    HdrKind.None, HdrKind.Unknown -> null
}

private fun bitrateLabel(kbps: Int?): String? {
    if (kbps == null || kbps <= 0) return null
    return if (kbps < 1000) "$kbps kbps" else "%.1f Mbps".format(kbps / 1000f)
}

private fun audioLabel(track: AudioTrackInfo): String {
    val layout: String? = when (track.channels) {
        1 -> "1.0"
        2 -> "2.0"
        6 -> "5.1"
        8 -> "7.1"
        else -> if (track.channels > 0) "${track.channels}ch" else null
    }
    return if (layout == null) track.codec else "${track.codec} $layout"
}

@Composable
private fun TechPill(badge: TechBadgeSpec, fg: Color) {
    val shape = RoundedCornerShape(BadgeCorner)
    Box(
        modifier = Modifier
            .height(BadgeHeight)
            .clip(shape)
            .background(
                if (badge.strong) DesignTokens.accentGreen.copy(alpha = StrongFillAlpha)
                else DesignTokens.groupedFill
            )
            .border(BorderStrokeWidth, DesignTokens.hairline, shape)
            .padding(horizontal = BadgeSidePad),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = badge.text,
            fontSize = BadgeFontSize,
            fontWeight = FontWeight.Medium,
            color = if (badge.strong) DesignTokens.accentGreen else fg,
            maxLines = 1,
        )
    }
}

private val BadgeGap = 6.dp
private val BadgeHeight = 24.dp
private val BadgeCorner = 12.dp
private val BadgeSidePad = 8.dp
private val BadgeFontSize = 11.sp
private val BorderStrokeWidth = 0.5.dp
private const val StrongFillAlpha = 0.14f

// ───────────────────────────── Preview ─────────────────────────────

@Composable
private fun PreviewOnDetailCanvas(content: @Composable () -> Unit) {
    Box(Modifier.background(DesignTokens.pageBackground).padding(13.dp)) { content() }
}

private fun previewInfo(
    width: Int, height: Int, codec: String?, depth: Int?,
    kbps: Int?, hdr: HdrKind, audio: List<AudioTrackInfo>,
) = MediaTechInfo(width, height, codec, depth, kbps, hdr, audio)

/** 空态：`info = null` ⇒ 整行不渲染，画布只剩屏边（13dp×2）。 */
@Preview(widthDp = 360, heightDp = 26)
@Composable
private fun TechBadgesEmptyPreview() {
    PreviewOnDetailCanvas { TechBadges(info = null) }
}

/** 常态：1080P · AVC · 8bit · SDR · 2.0（四枚，单行）。 */
@Preview(widthDp = 360, heightDp = 50)
@Composable
private fun TechBadgesFhdPreview() {
    PreviewOnDetailCanvas {
        TechBadges(
            info = previewInfo(
                width = 1920, height = 1080, codec = "AVC", depth = 8, kbps = 4_800,
                hdr = HdrKind.None,
                audio = listOf(AudioTrackInfo("AAC", 2, "zh")),
            ),
        )
    }
}

/** 竖屏：1080×2400 短边 1080 ⇒ `1080P`（旧的长边/height 判定会误报 4K）。 */
@Preview(widthDp = 360, heightDp = 50)
@Composable
private fun TechBadgesPortraitPreview() {
    PreviewOnDetailCanvas {
        TechBadges(
            info = previewInfo(
                width = 1080, height = 2400, codec = "AVC", depth = 8, kbps = 3_200,
                hdr = HdrKind.None,
                audio = listOf(AudioTrackInfo("AAC", 2, "zh")),
            ),
        )
    }
}

/** 满态：4K · HEVC · 10bit · HDR10 · 78.2Mbps · 5.1（6 枚，360dp 下折两行）。 */
@Preview(widthDp = 360, heightDp = 80)
@Composable
private fun TechBadgesUhdPreview() {
    PreviewOnDetailCanvas {
        TechBadges(
            info = previewInfo(
                width = 3840, height = 2160, codec = "HEVC", depth = 10, kbps = 78_200,
                hdr = HdrKind.HDR10,
                audio = listOf(AudioTrackInfo("EAC3", 6, "en")),
            ),
        )
    }
}