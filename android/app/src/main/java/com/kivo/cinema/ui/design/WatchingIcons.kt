package com.kivo.cinema.ui.design

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp

/**
 * 实心垃圾桶（对齐 SF Symbols `trash.fill`）：提手 + 盖条 + 收腰桶身 + 两条镂空竖纹。
 * [tint] 为图标色（白），[cutout] 为镂空色（红区底色），让竖纹透出红色。
 */
@Composable
fun TrashIcon(tint: Color, cutout: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val corner = CornerRadius(1.6.dp.toPx())
        // 提手
        drawRoundRect(tint, topLeft = Offset(w * 0.42f, h * 0.06f),
            size = Size(w * 0.16f, h * 0.10f), cornerRadius = corner)
        // 盖条
        drawRoundRect(tint, topLeft = Offset(w * 0.13f, h * 0.22f),
            size = Size(w * 0.74f, h * 0.10f), cornerRadius = corner)
        // 收腰桶身
        val body = Path().apply {
            moveTo(w * 0.22f, h * 0.38f)
            lineTo(w * 0.78f, h * 0.38f)
            lineTo(w * 0.71f, h * 0.92f)
            quadraticBezierTo(w * 0.70f, h * 0.95f, w * 0.66f, h * 0.95f)
            lineTo(w * 0.34f, h * 0.95f)
            quadraticBezierTo(w * 0.30f, h * 0.95f, w * 0.29f, h * 0.92f)
            close()
        }
        drawPath(body, tint)
        // 两条镂空竖纹
        val sw = 1.7.dp.toPx()
        drawLine(cutout, Offset(w * 0.42f, h * 0.47f), Offset(w * 0.44f, h * 0.86f),
            strokeWidth = sw, cap = StrokeCap.Round)
        drawLine(cutout, Offset(w * 0.58f, h * 0.47f), Offset(w * 0.56f, h * 0.86f),
            strokeWidth = sw, cap = StrokeCap.Round)
    }
}

/** 细描边对勾：用于多选勾选圈，避免字体渲染的勾粗细不一致。 */
@Composable
fun CheckMarkIcon(tint: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val sw = 2.1.dp.toPx()
        drawLine(tint, Offset(w * 0.20f, h * 0.52f), Offset(w * 0.43f, h * 0.74f),
            strokeWidth = sw, cap = StrokeCap.Round)
        drawLine(tint, Offset(w * 0.43f, h * 0.74f), Offset(w * 0.82f, h * 0.27f),
            strokeWidth = sw, cap = StrokeCap.Round)
    }
}
