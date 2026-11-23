package com.kivo.cinema.ui.design.home.v6

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
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
import androidx.compose.ui.unit.dp
import com.kivo.cinema.library.MediaQualityInfo

/**
 * v6 的两个视觉原子：
 * - [SourceStrip]：继续观看卡左上的片源签条「4K · HDR10 · 书房 NAS」（画质段深青、来源段玻璃）；
 * - [QualityBadge]：海报左下的单枚画质标「4K HDR」（106dp 宽海报放不下三段签条）。
 *
 * 画质只来自 [MediaQualityInfo] 的文件名解析；认不出就不画，不猜。
 */
@Composable
internal fun SourceStrip(quality: List<String>, source: String, modifier: Modifier = Modifier) {
    Row(modifier.height(20.dp).clip(RoundedCornerShape(5.dp))) {
        quality.forEachIndexed { i, q ->
            Segment(q, if (i == 0) V6Metrics.quality else V6Metrics.qualityAlt)
        }
        if (source.isNotBlank()) Segment(source, V6Metrics.badgeGlass)
    }
}

@Composable
internal fun QualityBadge(text: String, modifier: Modifier = Modifier) {
    Box(
        modifier.height(18.dp).clip(RoundedCornerShape(5.dp)).background(V6Metrics.badgeGlass)
            .padding(horizontal = 6.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(text, color = Color.White, fontSize = V6Metrics.fsBadge, fontWeight = FontWeight.Bold)
    }
}

@Composable
private fun Segment(text: String, bg: Color) {
    Box(Modifier.height(20.dp).background(bg).padding(horizontal = 6.dp), contentAlignment = Alignment.Center) {
        Text(text, color = Color.White, fontSize = V6Metrics.fsBadge, fontWeight = FontWeight.Bold, maxLines = 1)
    }
}

/** 文件名 → 签条画质段（「4K」「HDR10」）；SDR 不写，认不出返回空表。 */
internal fun qualitySegments(fileName: String): List<String> {
    val q = MediaQualityInfo.from(fileName)
    return listOfNotNull(shortRes(q.resolution), shortRange(q.dynamicRange))
}

/** 文件名 → 海报单枚画质标（「4K HDR」「4K DV」）；认不出分辨率返回 null。 */
internal fun qualityBadgeText(fileName: String): String? {
    val q = MediaQualityInfo.from(fileName)
    val res = shortRes(q.resolution) ?: return null
    val range = shortRange(q.dynamicRange)?.let { if (it == "DV") "DV" else "HDR" }
    return if (range != null) "$res $range" else res
}

/** 「1080P」→「1080p」：小写 p 是影视圈通行写法，大写在小字号下像错字。 */
private fun shortRes(res: String): String? = res.ifBlank { null }?.replace("P", "p")

/** 「DOLBY VISION」→「DV」；SDR 不写。 */
private fun shortRange(range: String): String? = when (range) {
    "", "SDR" -> null
    "DOLBY VISION" -> "DV"
    else -> range
}
