package com.kivo.cinema.danmaku

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameMillis
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.unit.dp
import kotlin.math.min
import kotlinx.coroutines.delay

/**
 * 弹幕滚动层（223 设计稿 D2）：Compose Canvas overlay。
 * withFrameMillis 驱动帧循环（每帧重绘 ~60fps）。
 */
@Composable
fun DanmakuOverlay(
    items: List<DanmakuItem>,
    positionMs: Long,
    visible: Boolean,
    modifier: Modifier = Modifier,
    speedSec: Float = 8f,
) {
    if (!visible || items.isEmpty()) return

    // 位置 MS 由外部传——但动画帧需要用 withFrameMillis 驱动重绘
    var frameTime by remember { mutableStateOf(0L) }
    LaunchedEffect(Unit) {
        while (true) {
            withFrameMillis { frameTime = it }
        }
    }

    val density = androidx.compose.ui.platform.LocalDensity.current.density
    val textSize = 14f * density

    Canvas(modifier = modifier) {
        val canvasWidth = size.width
        val canvasHeight = size.height
        val nowSec = positionMs / 1000f
        val wallSec = frameTime / 1000f

        val paint = android.graphics.Paint().apply { isAntiAlias = true }
        val trackHeight = textSize * 1.6f
        val maxTracks = min((canvasHeight / trackHeight).toInt(), 12)

        for (item in items) {
            val age = nowSec - item.time
            if (age < 0 || age > speedSec) continue
            val progress = age / speedSec
            val textW = paint.measureText(item.text)
            val x = canvasWidth - progress * (canvasWidth + textW)
            val trackIdx = (item.hashCode().toUInt() % maxTracks.toUInt()).toInt()
            val y = trackIdx * trackHeight + textSize

            paint.textSize = textSize * (item.size / 25f)
            // 黑描边
            paint.style = android.graphics.Paint.Style.STROKE
            paint.strokeWidth = 3f * density
            paint.color = android.graphics.Color.BLACK
            drawIntoCanvas { canvas ->
                canvas.nativeCanvas.drawText(item.text, x, y, paint)
            }
            // 填充
            paint.style = android.graphics.Paint.Style.FILL
            paint.color = android.graphics.Color.rgb(
                (item.color shr 16) and 0xFF,
                (item.color shr 8) and 0xFF,
                item.color and 0xFF
            )
            drawIntoCanvas { canvas ->
                canvas.nativeCanvas.drawText(item.text, x, y, paint)
            }
        }
    }
}
