package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeight
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.abs
import com.kivo.cinema.ui.compose.player.feedback.playerFeedbackSurface

/** 预览卡几何唯一来源：卡本体与 Popup 落位共用，避免两处各算一遍。 */
internal fun seekPreviewWidth(durationMs: Long, containerWidth: Dp): Dp =
    minOf(if (durationMs >= 3_600_000L) 224.dp else 190.dp, containerWidth)

internal fun seekPreviewHeight(width: Dp, showFrame: Boolean): Dp =
    if (showFrame) (width - 12.dp) * 9f / 16f + 68.dp else 62.dp

@Composable
internal fun PlayerSeekPreview(
    bitmap: Bitmap?,
    unavailableReason: String,
    targetMs: Long,
    originMs: Long,
    durationMs: Long,
    showFrame: Boolean,
    containerWidth: Dp,
    onCapture: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val width = seekPreviewWidth(durationMs, containerWidth)
    val height = seekPreviewHeight(width, showFrame)
    val frameHeight = (width - 12.dp) * 9f / 16f
    val delta = targetMs - originMs
    val direction = when { delta > 0 -> "向前"; delta < 0 -> "向后"; else -> "" }
    val deltaText = when {
        delta == 0L -> "保持当前位置"
        abs(delta) < 1_000L -> "${direction}不足 1 秒"
        else -> "$direction ${formatPlayerTime(abs(delta))}"
    }

    Column(
        modifier = modifier.width(width)
            .requiredHeight(height).playerFeedbackSurface(14.dp).padding(6.dp),
    ) {
        if (showFrame) {
            Box(
                modifier = Modifier.fillMaxWidth().height(frameHeight)
                    .clip(RoundedCornerShape(8.dp)).background(Color(0xFF182321)),
                contentAlignment = Alignment.Center,
            ) {
                if (bitmap != null && !bitmap.isRecycled) {
                    Image(
                        bitmap = bitmap.asImageBitmap(),
                        contentDescription = "目标 ${formatPlayerTime(targetMs)} 附近的预览画面",
                        contentScale = ContentScale.Crop,
                        modifier = Modifier.fillMaxWidth().height(frameHeight),
                    )
                } else {
                    Text(
                        unavailableReason.ifBlank { "预览帧准备中…" },
                        color = Color.White.copy(alpha = .65f), fontSize = 11.sp,
                        textAlign = TextAlign.Center, maxLines = 3,
                        overflow = TextOverflow.Ellipsis, modifier = Modifier.padding(8.dp),
                    )
                }
            }
        }
        Row(
            modifier = Modifier.fillMaxWidth().padding(start = 5.dp, top = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Row(
                    verticalAlignment = Alignment.Bottom,
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    Text(
                        formatPlayerTime(targetMs), color = Color(0xFFF2F5ED),
                        fontSize = if (durationMs >= 3_600_000L) 20.sp else 22.sp,
                        fontWeight = FontWeight.Medium, maxLines = 1,
                        style = TextStyle(fontFeatureSettings = "tnum"),
                    )
                    Text(
                        "/ ${formatPlayerTime(durationMs)}", color = Color.White.copy(alpha = .5f),
                        fontSize = 10.sp, maxLines = 1, modifier = Modifier.padding(bottom = 3.dp),
                    )
                }
                Text(deltaText, color = Color(0xFFBFCEB8), fontSize = 10.sp, maxLines = 1)
            }
            Box(
                modifier = Modifier.size(32.dp).clickable(role = Role.Button, onClickLabel = "截取当前播放画面", onClick = onCapture),
                contentAlignment = Alignment.Center,
            ) {
                PlayerGlyph("camera", color = PlayerTokens.textSecondary, modifier = Modifier.size(15.dp))
            }
        }
    }
}
