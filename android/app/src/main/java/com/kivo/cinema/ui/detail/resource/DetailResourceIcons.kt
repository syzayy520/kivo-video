package com.kivo.cinema.ui.detail

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp

/**
 * 详情页资源卡的四个矢量图标（对齐 REX 播放资源卡的符号语言）。
 * 与 `WatchingIcons` 同一套 Canvas 细描边风格；tint 由调用方传语义色，不写死色值。
 */

/** 软盘/容量图标：外框 + 顶部标签条 + 下方写入窗。 */
@Composable
internal fun StorageIcon(tint: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val sw = 1.5.dp.toPx()
        val r = CornerRadius(1.5.dp.toPx())
        drawRoundRect(tint, topLeft = Offset(sw / 2, sw / 2),
            size = Size(w - sw, h - sw), cornerRadius = r, style = Stroke(sw))
        // 顶部标签条
        drawLine(tint, Offset(w * 0.24f, h * 0.06f), Offset(w * 0.76f, h * 0.06f),
            strokeWidth = sw)
        drawLine(tint, Offset(w * 0.24f, h * 0.06f), Offset(w * 0.24f, h * 0.42f),
            strokeWidth = sw)
        drawLine(tint, Offset(w * 0.24f, h * 0.42f), Offset(w * 0.76f, h * 0.42f),
            strokeWidth = sw)
        drawLine(tint, Offset(w * 0.76f, h * 0.42f), Offset(w * 0.76f, h * 0.06f),
            strokeWidth = sw)
        // 写入窗
        drawRoundRect(tint, topLeft = Offset(w * 0.30f, h * 0.60f),
            size = Size(w * 0.40f, h * 0.30f), cornerRadius = r, style = Stroke(sw))
    }
}

/** 菱形/码率图标：REX 用钻石形表示码率。 */
@Composable
internal fun BitrateIcon(tint: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val sw = 1.5.dp.toPx()
        val diamond = Path().apply {
            moveTo(w * 0.5f, h * 0.06f)
            lineTo(w * 0.94f, h * 0.5f)
            lineTo(w * 0.5f, h * 0.94f)
            lineTo(w * 0.06f, h * 0.5f)
            close()
        }
        drawPath(diamond, tint, style = Stroke(sw))
    }
}

/** 实心小菱形：来源名前缀（REX 的绿色菱形）。 */
@Composable
internal fun SourceDiamond(tint: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val d = Path().apply {
            moveTo(w * 0.5f, 0f)
            lineTo(w, h * 0.5f)
            lineTo(w * 0.5f, h)
            lineTo(0f, h * 0.5f)
            close()
        }
        drawPath(d, tint)
    }
}

/** 绿色实心圆 + 白色对勾：表示「文件可用」，替代旧的文字 ✓。 */
@Composable
internal fun AvailableCheck(tint: Color, tick: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        drawCircle(tint, radius = minOf(w, h) / 2f)
        val sw = 1.9.dp.toPx()
        drawLine(tick, Offset(w * 0.28f, h * 0.51f), Offset(w * 0.45f, h * 0.67f),
            strokeWidth = sw, cap = androidx.compose.ui.graphics.StrokeCap.Round)
        drawLine(tick, Offset(w * 0.45f, h * 0.67f), Offset(w * 0.73f, h * 0.36f),
            strokeWidth = sw, cap = androidx.compose.ui.graphics.StrokeCap.Round)
    }
}
