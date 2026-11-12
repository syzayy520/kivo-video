package com.kivo.cinema.ui.detail.actions

import androidx.compose.foundation.Canvas
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp

/** 动作行线框图标（240 帧实测，从 DetailSections 按变化原因拆出）：收藏堆叠 + 心形。 */

/** 「加入收藏夹」图标：圆角方框描边 + 顶上一道堆叠线 + 左下圆环带加号。 */
@androidx.compose.runtime.Composable
internal fun CollectionPlusIcon(color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val stroke = 1.9.dp.toPx()
        drawLine(color, Offset(w * .20f, h * .10f), Offset(w * .80f, h * .10f),
            strokeWidth = stroke, cap = StrokeCap.Round)
        drawRoundRect(color, topLeft = Offset(w * .16f, h * .26f),
            size = androidx.compose.ui.geometry.Size(w * .68f, h * .60f),
            cornerRadius = androidx.compose.ui.geometry.CornerRadius(3.dp.toPx()),
            style = Stroke(stroke))
        val cx = w * .18f
        val cy = h * .82f
        val cr = w * .17f
        drawCircle(color, cr, Offset(cx, cy), style = Stroke(stroke))
        val pl = cr * .55f
        drawLine(color, Offset(cx - pl, cy), Offset(cx + pl, cy),
            strokeWidth = stroke * .85f, cap = StrokeCap.Round)
        drawLine(color, Offset(cx, cy - pl), Offset(cx, cy + pl),
            strokeWidth = stroke * .85f, cap = StrokeCap.Round)
    }
}

/** 心形描边图标：空心、圆头描边；filled=true 实心。 */
@androidx.compose.runtime.Composable
internal fun HeartIcon(color: Color, modifier: Modifier = Modifier, filled: Boolean = false) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val p = Path().apply {
            moveTo(w * .5f, h * .93f)
            cubicTo(w * .10f, h * .62f, w * .03f, h * .33f, w * .24f, h * .17f)
            cubicTo(w * .38f, h * .06f, w * .48f, h * .18f, w * .5f, h * .27f)
            cubicTo(w * .52f, h * .18f, w * .62f, h * .06f, w * .76f, h * .17f)
            cubicTo(w * .97f, h * .33f, w * .90f, h * .62f, w * .5f, h * .93f)
            close()
        }
        if (filled) drawPath(p, color)
        else drawPath(p, color, style = Stroke(1.9.dp.toPx(),
            cap = StrokeCap.Round, join = StrokeJoin.Round))
    }
}