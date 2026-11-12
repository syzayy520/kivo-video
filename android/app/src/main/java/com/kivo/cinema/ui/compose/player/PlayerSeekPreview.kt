package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeight
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 拖动进度时的**预览浮窗**（v3 §1.5）。
 *
 * 实测：缩略图 167×90pt、白描边 2pt、圆角 8pt、中心跟随拖点；
 * 浮窗**左侧**一个 48pt 毛玻璃圆钮＝拖动中一键截取当前帧为剧照。
 *
 * 【为什么必须向上偏移】浮窗与轨道同层，只 offset(x) 的话它会从轨道顶部
 * **往下铺 90pt**，把按钮行整个盖住（v2 实机：底部文字被水平切断且点不到）。
 * 现在底边贴轨道上沿，且用 offset 而非 padding ⇒ **不占布局高度**。
 *
 * 【没有缩略图时画什么】画一个深色空框，而不是返回不画 ——
 * 浮窗的几何是参考的一部分，缺帧时保持形状、内容留空，
 * 比"有时有浮窗有时没有"更容易让用户理解正在发生什么。
 */
@Composable
internal fun PlayerSeekPreview(
    bitmap: Bitmap?,
    unavailableReason: String,
    fraction: Float,
    targetMs: Long,
    containerWidth: Dp,
    onCapture: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val popupWidth = PlayerTokens.previewWidth.dp
    val popupHeight = PlayerTokens.previewHeight.dp
    val camera = PlayerTokens.previewCameraSize.dp
    val gap = 8.dp
    val totalWidth = camera + gap + popupWidth
    // 贴边保护：两端不让浮窗跑出屏幕。
    val centerX = containerWidth * fraction.coerceIn(0f, 1f)
    val x = (centerX - totalWidth / 2).coerceIn(0.dp, (containerWidth - totalWidth).coerceAtLeast(0.dp))
    val shape = RoundedCornerShape(PlayerTokens.previewRadius.dp)

    Row(
        modifier = modifier
            .offset(x = x, y = -(popupHeight + gap))
            // 轨道宿主只有 trackHitHeight 高：不用 requiredHeight 的话
            // 传入约束会把 90pt 浮窗压成一条扁条（真机实测 54px）。
            .requiredHeight(popupHeight),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            modifier = Modifier
                .size(camera)
                .clip(CircleShape)
                .background(PlayerTokens.panelSurface)
                .border(1.dp, PlayerTokens.panelBorder, CircleShape)
                .pointerInput(Unit) { detectTapGestures(onTap = { onCapture() }) },
            contentAlignment = Alignment.Center,
        ) {
            PlayerGlyph(
                kind = "camera",
                color = PlayerTokens.textPrimary,
                modifier = Modifier.size(22.dp),
            )
        }
        Box(
            modifier = Modifier
                .padding(start = gap)
                .size(width = popupWidth, height = popupHeight)
                .clip(shape)
                .background(Color(0xFF101012))
                .border(PlayerTokens.previewBorder.dp, Color.White, shape),
        ) {
            if (bitmap != null) {
                Image(
                    bitmap = bitmap.asImageBitmap(),
                    contentDescription = "拖动目标 ${formatPlayerTime(targetMs)} 的画面",
                    contentScale = ContentScale.Crop,
                    modifier = Modifier.size(width = popupWidth, height = popupHeight),
                )
            } else if (unavailableReason.isNotEmpty()) {
                // 不许静默降级：产不出帧就把原因画进空框，而不是让用户对着黑框猜。
                Text(
                    text = unavailableReason,
                    color = Color.White.copy(alpha = 0.6f),
                    fontSize = 11.sp,
                    textAlign = TextAlign.Center,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier
                        .align(Alignment.Center)
                        .padding(6.dp),
                )
            }
        }
    }
}