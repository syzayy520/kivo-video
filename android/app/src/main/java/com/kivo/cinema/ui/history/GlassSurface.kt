package com.kivo.cinema.ui.history

import androidx.compose.foundation.background
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 玻璃材质 Modifier（视觉稿 glass.css）：半透明白底 + 顶部内高光 + 1dp 斜向渐变亮边 + 柔影。
 *
 * [onImage] = 压在画面上的玻璃（「继续」胶囊）：白 26%、亮边更透，影子更深。
 * 项目里没有 backdrop 模糊管线（REX overlay 同口径），玻璃 = 半透明 + 亮边，不做背景取样。
 */
internal fun Modifier.glassSurface(shape: Shape, onImage: Boolean = false): Modifier {
    val m = HistoryMetrics
    val fill = if (onImage) m.glassOnImage else m.glassLight
    val edge = if (onImage) .55f else .95f
    return this
        .shadow(
            if (onImage) 4.dp else 8.dp, shape,
            ambientColor = DesignTokens.textMain.copy(alpha = .16f),
            spotColor = DesignTokens.textMain.copy(alpha = .16f),
        )
        .clip(shape)
        .background(fill)
        .drawBehind {
            // 内高光：顶部压亮上缘（视觉稿 inset 0 1px 0 white/.95）
            drawRect(
                Brush.verticalGradient(
                    0f to DesignTokens.canvasFill.copy(alpha = if (onImage) .55f else .95f),
                    .12f to Color.Transparent,
                ),
            )
            val rounded = shape as? RoundedCornerShape ?: return@drawBehind
            drawGlassEdge(rounded, edge)
        }
}

/** 沿圆角描一圈 1dp 斜向渐变边：左上最亮 → 中段淡 → 右下回亮（视觉稿 glass::after）。 */
private fun DrawScope.drawGlassEdge(shape: RoundedCornerShape, edgeAlpha: Float) {
    val brush = Brush.linearGradient(
        0f to DesignTokens.canvasFill.copy(alpha = edgeAlpha),
        .45f to DesignTokens.canvasFill.copy(alpha = edgeAlpha * .25f),
        1f to DesignTokens.canvasFill.copy(alpha = edgeAlpha * .65f),
        start = Offset.Zero,
        end = Offset(size.width, size.height),
    )
    val r = shape.topStart.toPx(size, this)
    drawRoundRect(
        brush,
        cornerRadius = androidx.compose.ui.geometry.CornerRadius(r, r),
        style = Stroke(1.dp.toPx()),
    )
}