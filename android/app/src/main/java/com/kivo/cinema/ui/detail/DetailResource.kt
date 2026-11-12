package com.kivo.cinema.ui.detail

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.EaseInOut
import androidx.compose.animation.core.tween
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 播放资源呈现族（254 D14，从 DetailSections 按变化原因拆出）。
 * **一套卡服务两种来源**：本地 [MediaMatch.Matched] 与 Emby `EmbyMediaInfo` 各映射成
 * [ResourceMedia] 喂同一个 [MediaCard]（「同一个东西的多份数据，不是多个文件」）。
 * 渲染像素与原 MediaCard 一致；展开媒体信息升级为可点击（有 streams 才可展开，无则不渲染死 affordance）。
 */
internal data class ResourceMedia(
    val resolution: String,
    val sizeBytes: Long,
    val bitrateBps: Long,
    val sourceLabel: String,
    val streams: ResourceStreams?,
)

internal data class ResourceStreams(
    val container: String,
    val video: List<String>,
    val audio: List<String>,
    val subtitles: List<String>,
)

/** 播放资源行（240 §9.3）：标题 + 清晰度胶囊。 */
@Composable
internal fun PlaySourceHeader(quality: String, fg: Color) {
    Row(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("播放资源:", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = fg)
        Spacer(Modifier.width(12.dp))
        Text(
            quality, fontSize = 15.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            modifier = Modifier
                .clip(RoundedCornerShape(50))
                .background(DesignTokens.canvasFill)
                .padding(horizontal = 18.dp, vertical = 6.dp),
        )
    }
}

/** 媒体信息卡（240 §9.3）：白卡 = 分辨率 / 容量·码率 / 来源名+绿勾。 */
@Composable
internal fun MediaCard(m: ResourceMedia) {
    Column(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth()
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = 14.dp, vertical = 14.dp),
    ) {
        Text(m.resolution, fontSize = 17.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(8.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(formatSize(m.sizeBytes), fontSize = 13.sp, color = DesignTokens.textSub)
            Spacer(Modifier.width(14.dp))
            Text(formatBitrate(m.bitrateBps), fontSize = 13.sp, color = DesignTokens.textSub)
        }
        Spacer(Modifier.height(10.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.size(7.dp).clip(CircleShape).background(Color(0xFF34C759)))
            Spacer(Modifier.width(6.dp))
            Text(m.sourceLabel, fontSize = 13.sp, color = DesignTokens.textMain)
            Spacer(Modifier.weight(1f))
            Text("✓", fontSize = 15.sp, color = Color(0xFF34C759))
        }
    }
}

/** 展开媒体信息（254 D14）：有 streams 才可点击展开（容器/视频/音频/字幕真值）；无则不渲染。 */
@Composable
internal fun ExpandMediaInfoRow(fgSub: Color, streams: ResourceStreams?) {
    if (streams == null) return
    var expanded by remember { mutableStateOf(false) }
    Column(Modifier.fillMaxWidth()) {
        Row(Modifier.fillMaxWidth().clickable { expanded = !expanded }.padding(top = 12.dp),
            horizontalArrangement = Arrangement.Center, verticalAlignment = Alignment.CenterVertically) {
            Text(if (expanded) "收起媒体信息" else "展开媒体信息", fontSize = 13.sp, color = fgSub)
            Spacer(Modifier.width(6.dp))
            Text(if (expanded) "⌃" else "⌄", fontSize = 13.sp, color = fgSub)
        }
        AnimatedVisibility(visible = expanded,
            enter = expandVertically(tween(300, easing = EaseInOut)) + fadeIn(tween(300, easing = EaseInOut)),
            exit = shrinkVertically(tween(300, easing = EaseInOut)) + fadeOut(tween(300, easing = EaseInOut))) {
            Column(Modifier.padding(horizontal = DETAIL_PAD_DP.dp).padding(top = 10.dp),
                verticalArrangement = Arrangement.spacedBy(6.dp)) {
                if (streams.container.isNotBlank()) MediaInfoLine("容器", streams.container, fgSub)
                MediaInfoLines("视频", streams.video, fgSub)
                MediaInfoLines("音频", streams.audio, fgSub)
                MediaInfoLines("字幕", streams.subtitles, fgSub)
            }
        }
    }
}

@Composable
private fun MediaInfoLines(label: String, values: List<String>, fgSub: Color) {
    if (values.isNotEmpty()) MediaInfoLine(label, values.joinToString(" · "), fgSub)
}

@Composable
private fun MediaInfoLine(label: String, value: String, fgSub: Color) {
    Row {
        Text("$label  ", fontSize = 13.sp, color = fgSub, fontWeight = FontWeight.Bold)
        Text(value, fontSize = 13.sp, color = fgSub)
    }
}

/** 空态块（无本地资源，240 §6.1）：TV 图标 + 暂无可用资源 + 添加到待看。 */
@Composable
internal fun EmptyResourceBlock(fg: Color, watchlisted: Boolean, enabled: Boolean, onWatchlist: () -> Unit) {
    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Canvas(Modifier.size(86.dp, 64.dp)) {
            val w = size.width
            val h = size.height
            val bodyTop = h * .16f
            drawLine(fg.copy(alpha = .22f), Offset(w * .38f, bodyTop), Offset(w * .26f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = StrokeCap.Round)
            drawLine(fg.copy(alpha = .22f), Offset(w * .62f, bodyTop), Offset(w * .74f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = StrokeCap.Round)
            drawRoundRect(fg.copy(alpha = .16f), topLeft = Offset(0f, bodyTop),
                size = androidx.compose.ui.geometry.Size(w, h - bodyTop),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(9.dp.toPx()))
            val pad = 9.dp.toPx()
            drawRoundRect(fg.copy(alpha = .26f),
                topLeft = Offset(pad, bodyTop + pad),
                size = androidx.compose.ui.geometry.Size(w - pad * 2, h - bodyTop - pad * 2),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(5.dp.toPx()))
            val cx = w / 2f
            val cy = bodyTop + (h - bodyTop) / 2f
            val r = 9.dp.toPx()
            val tri = Path().apply {
                moveTo(cx - r * .5f, cy - r)
                lineTo(cx + r, cy)
                lineTo(cx - r * .5f, cy + r)
                close()
            }
            drawPath(tri, fg.copy(alpha = .85f))
        }
        Spacer(Modifier.height(14.dp))
        Text("暂无可用资源", fontSize = 15.sp, fontWeight = FontWeight.Bold, color = fg)
        Spacer(Modifier.height(6.dp))
        Text(if (watchlisted) "已添加到待看 · 点击取消" else "添加到待看",
            fontSize = 12.sp, color = fg.copy(alpha = .75f),
            modifier = Modifier.clickable(enabled = enabled,
                role = androidx.compose.ui.semantics.Role.Button, onClick = onWatchlist)
                .padding(horizontal = 16.dp, vertical = 14.dp))
    }
}

internal fun formatSize(bytes: Long): String = when {
    bytes >= 1073741824L -> "%.1f GB".format(bytes / 1073741824.0)
    bytes >= 1048576L -> "%.0f MB".format(bytes / 1048576.0)
    else -> "%.0f KB".format(bytes / 1024.0)
}

internal fun formatBitrate(bps: Long): String =
    if (bps >= 1_000_000L) "%.2fMbps".format(bps / 1_000_000.0)
    else "%.0fkbps".format(bps / 1000.0)