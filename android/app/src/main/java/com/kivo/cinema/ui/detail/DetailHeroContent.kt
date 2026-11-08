package com.kivo.cinema.ui.detail

import android.content.Context
import android.widget.Toast
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 片名图层：独立于艺术图，贴在 meta 上方；与 meta 同属内容层，一起离屏。 */
@Composable
internal fun TitleLayer(d: TmdbDetail.Details, fg: Color) {
    Box(
        Modifier.fillMaxWidth().height(DETAIL_TITLE_BAND_DP.dp).padding(horizontal = 24.dp),
        contentAlignment = Alignment.BottomCenter,
    ) {
        val logo = d.logoUrl
        if (logo != null) {
            TmdbImage(logo, d.title, Modifier.fillMaxSize(), bg = Color.Transparent, fit = true)
        } else {
            Text(d.title, fontSize = 34.sp, fontWeight = FontWeight.Bold, color = fg,
                textAlign = TextAlign.Center, maxLines = 2,
                modifier = Modifier.fillMaxWidth())
        }
    }
}

/** meta 行：上层底色的起点（日期上方 18dp）。 */
@Composable
internal fun MetaLine(d: TmdbDetail.Details, fg: Color) {
    Text(
        text = d.metaLine(),
        fontSize = 13.sp, lineHeight = 13.sp,
        color = fg, textAlign = TextAlign.Center,
        modifier = Modifier.fillMaxWidth()
            .padding(start = DETAIL_PAD_DP.dp, end = DETAIL_PAD_DP.dp, top = 18.dp),
    )
}

@Composable
internal fun Overview(text: String, fg: Color) {
    Text(
        text = text,
        fontSize = 15.sp, lineHeight = 14.sp,
        color = fg.copy(alpha = 0.82f),
        maxLines = 2, overflow = TextOverflow.Ellipsis,
        modifier = Modifier.fillMaxWidth()
            .padding(start = DETAIL_PAD_DP.dp, end = DETAIL_PAD_DP.dp, top = 16.dp),
    )
}

@Composable
internal fun ActionRow(context: Context, media: MediaMatch.Matched?, fg: Color, onPlay: (MediaMatch.Matched) -> Unit) {
    val reason = "未加载到相关资源"
    val hair = Color.White.copy(alpha = .10f)
    val glyph = fg.copy(alpha = .95f)
    val playable = media != null
    Row(
        Modifier.fillMaxWidth()
            .padding(start = DETAIL_PAD_DP.dp, end = DETAIL_PAD_DP.dp, top = 10.dp),
        horizontalArrangement = Arrangement.spacedBy(9.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            Modifier.weight(1f).height(45.dp)
                .clip(RoundedCornerShape(50))
                .background(Color.White.copy(alpha = if (playable) .92f else .60f))
                .border(1.dp, hair, RoundedCornerShape(50))
                .clickable {
                    if (media != null) onPlay(media)
                    else Toast.makeText(context, reason, Toast.LENGTH_SHORT).show()
                },
            contentAlignment = Alignment.Center,
        ) {
            Text(if (playable) "▶  播放" else reason,
                fontSize = 15.sp, fontWeight = FontWeight.Medium,
                color = DesignTokens.textMain,
                maxLines = 1, overflow = TextOverflow.Ellipsis)
        }
        Row(
            Modifier.width(101.dp).height(45.dp)
                .clip(RoundedCornerShape(50))
                .background(Color.White.copy(alpha = .147f))
                .border(1.dp, hair, RoundedCornerShape(50)),
            horizontalArrangement = Arrangement.Center,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            CollectionPlusIcon(glyph, Modifier.size(22.dp))
            Spacer(Modifier.width(15.dp))
            HeartIcon(glyph, Modifier.size(20.dp))
        }
        Box(Modifier.size(45.dp).clip(CircleShape)
            .background(Color.White.copy(alpha = .147f))
            .border(1.dp, hair, CircleShape),
            contentAlignment = Alignment.Center) {
            Canvas(Modifier.size(20.dp, 4.dp)) {
                val r = 2.0.dp.toPx()
                val gap = (size.width - r * 2) / 2f
                for (i in 0..2) {
                    drawCircle(glyph, r,
                        Offset(r + gap * i, size.height / 2f))
                }
            }
        }
    }
}

@Composable
internal fun BackCircle(onBack: () -> Unit) {
    Box(
        Modifier.padding(start = DETAIL_PAD_DP.dp, top = 42.dp)
            .size(42.dp)
            .clip(CircleShape)
            .background(Color(0x8CFFFFFF))
            .clickable { onBack() },
        contentAlignment = Alignment.Center,
    ) {
        Canvas(Modifier.size(11.dp, 17.dp)) {
            val p = Path().apply {
                moveTo(size.width, 0f)
                lineTo(0f, size.height / 2f)
                lineTo(size.width, size.height)
            }
            drawPath(p, Color(0xFF1A1A1A), style = Stroke(
                width = 2.2.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
        }
    }
}

/** Title band starts at 404.5dp; metadata begins at 510.5dp. */
internal const val DETAIL_TITLE_TOP_DP = 404.5f
/** 片名图层高度带。 */
private const val DETAIL_TITLE_BAND_DP = 88f
/** 节标题字号（同串墨高实测 18sp，非首页的 22sp）。 */
internal const val DETAIL_SECTION_TITLE_SP = 18f
/** 正文左右缩进。 */
internal const val DETAIL_PAD_DP = 20f
/** 节与节之间。 */
internal const val DETAIL_SECTION_GAP_DP = 42f
private fun TmdbDetail.Details.metaLine(): String {
val parts = mutableListOf<String>()
dateCn()?.let { parts.add(it) }
region?.let { parts.add(it) }
if (isTv) episodes?.let { parts.add("${it}集") }
else runtimeMin?.let { parts.add(runtimeText(it)) }
return parts.joinToString(" · ")
}

private fun TmdbDetail.Details.dateCn(): String? {
if (date.isEmpty()) return null
val p = date.split("-")
if (p.size != 3) return date
val m = p[1].toIntOrNull() ?: return date
val d = p[2].toIntOrNull() ?: return date
return "${p[0]}年${m}月${d}日"
}

private fun runtimeText(min: Int): String {
if (min < 60) return "${min}分钟"
val h = min / 60
val m = min % 60
return if (m == 0) "${h}小时" else "${h}小时${m}分钟"
}
