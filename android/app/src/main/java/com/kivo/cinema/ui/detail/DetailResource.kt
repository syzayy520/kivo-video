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
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
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
    /** 卡片主标题（动态范围档，如 SDR / DOLBY VISION）；空则回落显示 resolution。 */
    val qualityTitle: String = "",
)

internal data class ResourceStreams(
    val container: String,
    val video: List<String>,
    val audio: List<String>,
    val subtitles: List<String>,
)

/** 播放资源行（240 §9.3）：标题 + 清晰度胶囊；quality 为空时**不画胶囊**（消灭空圈）。 */
@Composable
internal fun PlaySourceHeader(quality: String, fg: Color) {
    Row(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("播放资源:", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = fg)
        if (quality.isNotBlank()) {
            Spacer(Modifier.width(12.dp))
            Text(
                quality, fontSize = 14.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
                modifier = Modifier
                    .clip(RoundedCornerShape(50))
                    .background(DesignTokens.canvasFill.copy(alpha = 0.92f))
                    .padding(horizontal = 16.dp, vertical = 6.dp),
            )
        }
    }
}

/**
 * 媒体信息卡（对齐 REX 播放资源卡）：圆角白卡 = 画质主标题 / 图标化容量·码率 / 绿菱形来源 + 圆勾。
 *
 * 视觉层级（从上到下）：
 * 1. 主标题 = 动态范围档（SDR / DOLBY VISION），18sp Bold —— REX 卡首行是 "SDR"/"Dolby" 大字。
 * 2. 容量 + 码率：软盘/菱形矢量图标 + 13sp 灰字，图标与字基线对齐。
 * 3. 来源：绿色实心菱形 + 来源名；行末绿色圆形对勾表示「文件可用」。
 *
 * 【为什么是"小豆腐块"而非通栏】REX 的卡宽只包住内容（约 240pt @430 屏），不铺满整行。
 * 用固定 `CardWidth` 而非 `IntrinsicSize.Min`：后者与 softWrap=false 的文字配合会把码率
 * 截成 "7.82M"（intrinsic 测量没把不换行文字的完整宽度计入）。固定宽度稳定且紧凑。
 * 全部用 DesignTokens 语义色，不写死色值（UI 门禁裸色约束）。
 */
@Composable
internal fun MediaCard(m: ResourceMedia) {
    val title = m.qualityTitle.ifBlank { m.resolution }
    Column(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).width(CardWidth)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .border(0.5.dp, DesignTokens.hairline, RoundedCornerShape(DesignTokens.radiusCard.dp))
            .padding(horizontal = 16.dp, vertical = 14.dp),
    ) {
        // 1) 画质主标题（动态范围档）。分辨率已在页级胶囊显示，卡内不再重复第二个 1080P。
        //    无文件名档位时（本地/Emby 有真实宽高）才回落到 resolution 当主标题。
        if (title.isNotBlank()) {
            Text(title, fontSize = 18.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain)
        }
        // 2) 容量 + 码率（图标化）—— 这一行决定卡片宽度
        if (m.sizeBytes > 0 || m.bitrateBps > 0) {
            Spacer(Modifier.height(10.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                if (m.sizeBytes > 0) {
                    StorageIcon(DesignTokens.textSub, Modifier.size(14.dp))
                    Spacer(Modifier.width(6.dp))
                    Text(formatSize(m.sizeBytes), fontSize = 13.sp, color = DesignTokens.textSub)
                }
                if (m.bitrateBps > 0) {
                    Spacer(Modifier.width(12.dp))
                    BitrateIcon(DesignTokens.textSub, Modifier.size(14.dp))
                    Spacer(Modifier.width(5.dp))
                    Text(formatBitrate(m.bitrateBps), fontSize = 13.sp, color = DesignTokens.textSub)
                }
            }
        }
        // 3) 来源 + 可用圆勾（fillMaxWidth 让绿勾落在卡右缘，与 REX 一致）
        if (m.sourceLabel.isNotBlank()) {
            Spacer(Modifier.height(11.dp))
            Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                SourceDiamond(DesignTokens.accentGreen, Modifier.size(11.dp))
                Spacer(Modifier.width(7.dp))
                Text(m.sourceLabel, fontSize = 13.sp, fontWeight = FontWeight.Medium,
                    color = DesignTokens.textMain)
                Spacer(Modifier.weight(1f))
                AvailableCheck(DesignTokens.accentGreen, DesignTokens.canvasFill,
                    Modifier.size(19.dp))
            }
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

/** 资源小卡宽度（REX 约 240pt @430 屏 → 本屏 210dp）：紧凑豆腐块，且容得下码率整行。 */
private val CardWidth = 210.dp