package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * 拖动进度时的**预览浮窗**（player-ui-v2.md §3.2）。
 *
 * ## 为什么单独一个文件
 * 它是"拖动时的一次性反馈"，与底栏的常态布局是**两种不同的东西**：
 * 常态底栏永远在，浮窗只在拖动期间存在。放在一起会让底栏文件同时管
 * "布局"和"瞬态反馈"两件事 —— 两者变化的原因不同（一个跟分辨率走，
 * 一个跟拖动手势走），所以按变化原因拆开。
 *
 * ## 关键：浮窗必须**向上**偏移
 * 第一版浮窗与进度条同层、只 `offset(x)`，y 保持 0 ⇒ 它从进度条顶部
 * **往下铺 90dp**，把下方的按钮行整个盖住。实机表现：
 * 底部文字只剩上半截（y=809 处被水平切断），且按钮点不到。
 *
 * 现在：向上偏移（底边贴着进度条上沿），**且不占布局高度**（用 offset 而非 padding）。
 *
 * 尺寸与描边对齐参考实测（167×90pt，白描边 2pt，圆角 8pt）。
 */
@Composable
internal fun PlayerSeekPreview(
    bitmap: Bitmap?,
    fraction: Float,
    containerWidth: Dp,
    modifier: Modifier = Modifier,
) {
    if (bitmap == null) return
    val popupWidth = 167.dp
    val popupHeight = 90.dp
    // 贴边保护：0.02/0.98 让浮窗在两端不会跑出屏幕。
    val centerX = containerWidth * fraction.coerceIn(0.02f, 0.98f)
    val popupX = (centerX - popupWidth / 2).coerceIn(0.dp, containerWidth - popupWidth)
    Image(
        bitmap = bitmap.asImageBitmap(),
        contentDescription = "Seek preview",
        contentScale = ContentScale.Crop,
        modifier = modifier
            .offset(x = popupX, y = -(popupHeight + 6.dp))
            .size(width = popupWidth, height = popupHeight)
            .clip(RoundedCornerShape(8.dp))
            .border(2.dp, Color.White, RoundedCornerShape(8.dp)),
    )
}
