package com.kivo.cinema.ui.compose

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 控制层（212 设计稿 D4）：底部渐变 scrim + 进度条 + 时间 + 播放/暂停。
 * Rex 视觉 token：深色 scrim + 白色控件 + 无多余 chrome（store-2/3）。
 *
 * 215 拖动预览（家族树"小改"）：
 * - 拖动中只更新本地 [dragValueMs] 并经 [onScrub] 请求缩略图——不打主链路；
 * - 松手（[Slider.onValueChangeFinished]）才提交 [onSeek]；
 * - 浮窗规格对齐 Rex 实测（05 号文档 §6.2）：16:9 白描边圆角卡，中心随拖点。
 */
@Composable
fun PlayerControls(
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    onPlayPause: () -> Unit,
    onSeek: (Long) -> Unit,
    onScrub: (Long) -> Unit = {},
    thumbnailBitmap: Bitmap? = null,
    previewMs: Long = -1L,   // ≥0：程序化预览（取证 harness 用）；与拖动共用同一条浮窗 UI
    danmakuVisible: Boolean = false,
    onDanmakuToggle: () -> Unit = {},
    casting: Boolean = false,
    onCastClick: () -> Unit = {},
    modifier: Modifier = Modifier,
) {
    var dragValueMs by remember { mutableStateOf(0L) }
    var dragging by remember { mutableStateOf(false) }
    val previewing = previewMs >= 0
    val shownMs = when {
        dragging -> dragValueMs
        previewing -> previewMs
        else -> positionMs
    }
    val fraction = if (durationMs > 0) shownMs.toFloat() / durationMs else 0f

    Column(
        modifier = modifier
            .fillMaxWidth()
            .background(
                Brush.verticalGradient(
                    colors = listOf(Color.Transparent, Color.Black.copy(alpha = 0.7f))
                )
            )
            .padding(horizontal = 16.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.Bottom,
    ) {
        // 缩略图浮窗（215）：中心跟随拖点，白色描边 + 圆角（Rex 05 §6.2）
        androidx.compose.foundation.layout.BoxWithConstraints(
            modifier = Modifier
                .fillMaxWidth()
                .height(96.dp),
        ) {
            val containerWidth = maxWidth
            val popupWidth = 160.dp
            val clampedFraction = fraction.coerceIn(0.02f, 0.98f)
            val centerX = containerWidth * clampedFraction
            val popupX = (centerX - popupWidth / 2).coerceIn(0.dp, containerWidth - popupWidth)
            if (dragging || previewing) thumbnailBitmap?.let { bmp ->
                Image(
                    bitmap = bmp.asImageBitmap(),
                    contentDescription = "Seek preview",
                    contentScale = ContentScale.Crop,
                    modifier = Modifier
                        .offset(x = popupX)
                        .size(width = popupWidth, height = 90.dp)
                        .clip(RoundedCornerShape(8.dp))
                        .border(2.dp, Color.White, RoundedCornerShape(8.dp)),
                )
            }
        }
        // 进度条
        Slider(
            value = fraction,
            onValueChange = { f ->
                dragging = true
                dragValueMs = (f * durationMs).toLong()
                onScrub(dragValueMs)
            },
            onValueChangeFinished = {
                dragging = false
                onSeek(dragValueMs)
            },
            colors = SliderDefaults.colors(
                thumbColor = Color.White,
                activeTrackColor = Color.White,
                inactiveTrackColor = Color.White.copy(alpha = 0.3f),
            ),
            modifier = Modifier.fillMaxWidth(),
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            // 播放/暂停按钮
            IconButton(onClick = onPlayPause) {
                Icon(
                    painter = androidx.compose.ui.res.painterResource(
                        if (isPlaying) android.R.drawable.ic_media_pause
                        else android.R.drawable.ic_media_play
                    ),
                    contentDescription = if (isPlaying) "Pause" else "Play",
                    tint = Color.White,
                )
            }
            // 弹幕开关（223）：圆圈"弹"字（Rex 同款）
            IconButton(onClick = onDanmakuToggle) {
                Text(
                    "弹",
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Bold,
                    color = if (danmakuVisible) Color(0xFF00A1D6) else Color(0xFF666666),
                    modifier = Modifier
                        .background(
                            if (danmakuVisible) Color(0xFF00A1D6).copy(alpha = 0.2f)
                            else Color.Transparent,
                            RoundedCornerShape(50)
                        )
                        .padding(4.dp)
                )
            }
            // 投屏按钮（230）：与「弹」同排同规格（Rex 05 token）
            IconButton(onClick = onCastClick) {
                Text(
                    "投",
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Bold,
                    color = if (casting) Color(0xFF34C759) else Color(0xFF666666),
                    modifier = Modifier
                        .background(
                            if (casting) Color(0xFF34C759).copy(alpha = 0.2f)
                            else Color.Transparent,
                            RoundedCornerShape(50)
                        )
                        .padding(4.dp)
                )
            }
            // 时间（拖动中显示拖动目标时间——对齐 Rex 05 §6.2）
            Text(
                text = "${formatTime(shownMs)} / ${formatTime(durationMs)}",
                color = Color.White,
                fontSize = 12.sp,
            )
        }
    }
}

private fun formatTime(ms: Long): String {
    val totalSec = ms / 1000
    return String.format("%d:%02d:%02d", totalSec / 3600, totalSec / 60 % 60, totalSec % 60)
}
