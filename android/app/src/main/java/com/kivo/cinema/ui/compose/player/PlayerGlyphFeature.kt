package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate

/**
 * 播放页的**功能入口图标**（对应 REX 顶栏那一排）。
 *
 * ## 为什么按 REX 的形状重画，而不是自己发挥
 * 用户明确要求："图标你要对比下 REX 的图标，就知道问题出在哪里了"。
 * 第一版画的是"方框+竖线""喇叭+声波"这类**自创**符号，
 * 而 REX 用的是一套**成体系的符号语言**：
 * | REX 图标 | 含义 | 形状 |
 * | --- | --- | --- |
 * | 场记板 | 画质比例 | 斜顶板 + 斜条纹 + 矩形机身 |
 * | 圈内双向箭头 | 画面缩放 | 圆 + 左上/右下两支箭头 |
 * | 圈内波形条 | 音轨 | 圆 + 5 根高低不等的竖条 |
 * | 对话气泡 | 字幕 | 圆角气泡（左下小尾巴）+ 两行横线 |
 * | 耳机 | 听视频 | 圆弧头梁 + 两只耳罩 |
 * | 投屏（DLNA） | 投屏 | 屏幕圆角框 + 左下信号弧 + 圆点（v3.1 替代 AirPlay） |
 * | 画中画 | PiP | 屏幕框 + 左上斜箭头 + 右下实心小方块 |
 * | 弹（圈内） | 弹幕 | 开口的圆 + 汉字"弹" |
 *
 * 【画法】全部是**线框 + 实心局部**的混搭，与 REX 一致；
 * 坐标系 24×24，与 REX 的符号比例接近。
 */
internal fun DrawScope.drawFeatureGlyph(kind: String, color: Color, stroke: Stroke) {
    when (kind) {
        // ---- 场记板（画质比例）----
        // 斜顶板 + 4 道斜条纹 + 下方圆角机身（带一道"铰链"横线）
        "clapper" -> {
            rotate(degrees = -13f, pivot = Offset(12f, 10.5f)) {
                drawRoundRect(
                    color = color,
                    topLeft = Offset(3.0f, 5.0f),
                    size = Size(18.0f, 5.4f),
                    cornerRadius = CornerRadius(1.0f),
                    style = stroke,
                )
                // 条纹：4 道斜杠（与参考一致的方向）
                var x = 6.6f
                while (x < 19.6f) {
                    drawLine(
                        color = color,
                        start = Offset(x, 5.6f),
                        end = Offset(x - 2.4f, 9.8f),
                        strokeWidth = stroke.width,
                        cap = StrokeCap.Round,
                    )
                    x += 3.8f
                }
            }
            // 机身
            drawRoundRect(
                color = color,
                topLeft = Offset(3.0f, 11.4f),
                size = Size(18.0f, 9.8f),
                cornerRadius = CornerRadius(1.8f),
                style = stroke,
            )
            // 铰链横线
            drawLine(
                color = color,
                start = Offset(3.4f, 14.6f),
                end = Offset(20.6f, 14.6f),
                strokeWidth = stroke.width,
                cap = StrokeCap.Round,
            )
        }

        // ---- 圈内双向箭头（画面缩放）----
        // 圆 + 两支**长**箭头（左上 / 右下），带完整箭杆与箭头
        "expand" -> {
            drawCircle(
                color = color,
                radius = 9.6f,
                center = Offset(12f, 12f),
                style = stroke,
            )
            // 左上箭头
            drawLine(color, Offset(13.3f, 10.7f), Offset(8.1f, 5.5f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(8.1f, 5.5f), Offset(10.6f, 5.8f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(8.1f, 5.5f), Offset(7.8f, 8.0f), stroke.width, StrokeCap.Round)
            // 右下箭头
            drawLine(color, Offset(10.7f, 13.3f), Offset(15.9f, 18.5f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(15.9f, 18.5f), Offset(13.4f, 18.2f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(15.9f, 18.5f), Offset(16.2f, 16.0f), stroke.width, StrokeCap.Round)
        }

        // ---- 圈内波形条（音轨，REX 0162）----
        // 圆 + 5 根高低不等的竖条。条宽必须小于条距，否则糊成一团实心。
        "audio" -> {
            drawCircle(
                color = color,
                radius = 9.6f,
                center = Offset(12f, 12f),
                style = stroke,
            )
            val heights = listOf(3.8f, 6.8f, 9.6f, 7.4f, 4.2f)
            var i = 0
            while (i < heights.size) {
                val x = 7.8f + i * 2.1f
                val h = heights[i]
                drawLine(
                    color = color,
                    start = Offset(x, 12f - h / 2f),
                    end = Offset(x, 12f + h / 2f),
                    strokeWidth = stroke.width * 0.75f,
                    cap = StrokeCap.Round,
                )
                i++
            }
        }

        // ---- 对话气泡（字幕）----
        // 宽圆角气泡 + 左下小尾巴 + 三行文字（一长两短，像一段字幕）
        "captions" -> {
            val bubble = Path().apply {
                moveTo(4.2f, 5.4f)
                lineTo(19.8f, 5.4f)
                quadraticTo(21.4f, 5.4f, 21.4f, 7.0f)
                lineTo(21.4f, 14.8f)
                quadraticTo(21.4f, 16.4f, 19.8f, 16.4f)
                lineTo(9.6f, 16.4f)
                lineTo(6.8f, 19.6f)   // 尾巴
                lineTo(6.8f, 16.4f)
                lineTo(4.2f, 16.4f)
                quadraticTo(2.6f, 16.4f, 2.6f, 14.8f)
                lineTo(2.6f, 7.0f)
                quadraticTo(2.6f, 5.4f, 4.2f, 5.4f)
                close()
            }
            drawPath(bubble, color, style = stroke)
            // 三行"文字"
            drawLine(color, Offset(5.6f, 8.8f), Offset(18.4f, 8.8f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(5.6f, 11.4f), Offset(12.4f, 11.4f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(14.4f, 11.4f), Offset(18.4f, 11.4f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(5.6f, 14.0f), Offset(10.4f, 14.0f), stroke.width, StrokeCap.Round)
        }

        // ---- 耳机（听视频，REX 0172）----
        // 圆弧头梁 + 两只实心耳罩
        "audioMode" -> {
            // 头梁：上半圆弧
            val band = Path().apply {
                moveTo(4.2f, 13.6f)
                quadraticTo(4.2f, 4.6f, 12f, 4.6f)
                quadraticTo(19.8f, 4.6f, 19.8f, 13.6f)
            }
            drawPath(band, color, style = stroke)
            // 耳罩（实心）
            drawRoundRect(
                color = color,
                topLeft = Offset(3.0f, 12.4f),
                size = Size(3.6f, 7.0f),
                cornerRadius = CornerRadius(1.8f),
            )
            drawRoundRect(
                color = color,
                topLeft = Offset(17.4f, 12.4f),
                size = Size(3.6f, 7.0f),
                cornerRadius = CornerRadius(1.8f),
            )
        }

        // ---- 投屏（DLNA，v3.1 替代 AirPlay）----
        // 屏幕圆角框（左下留缺口）+ 左下两道信号弧 + 实心圆点。
        // 不用 Apple 的实心三角符号：AirPlay 是专有协议，Android 不支持，整条已移除。
        "cast" -> {
            val screen = Path().apply {
                moveTo(6.6f, 16.6f)
                lineTo(19.4f, 16.6f)
                quadraticTo(21.2f, 16.6f, 21.2f, 14.8f)
                lineTo(21.2f, 6.4f)
                quadraticTo(21.2f, 4.6f, 19.4f, 4.6f)
                lineTo(4.6f, 4.6f)
                quadraticTo(2.8f, 4.6f, 2.8f, 6.4f)
                lineTo(2.8f, 10.4f)
            }
            drawPath(screen, color, style = stroke)
            // 信号弧：以左下角为心的两段四分之一弧
            drawArc(
                color = color,
                startAngle = -90f,
                sweepAngle = 90f,
                useCenter = false,
                topLeft = Offset(4.6f - 3.4f, 19.4f - 3.4f),
                size = Size(6.8f, 6.8f),
                style = stroke,
            )
            drawArc(
                color = color,
                startAngle = -90f,
                sweepAngle = 90f,
                useCenter = false,
                topLeft = Offset(4.6f - 6.4f, 19.4f - 6.4f),
                size = Size(12.8f, 12.8f),
                style = stroke,
            )
            drawCircle(color = color, radius = 1.3f, center = Offset(4.6f, 19.4f))
        }

        // ---- 画中画 PiP ----
        // 大屏线框 + 左上一支朝右下的实心箭头 + 右下一个实心圆角方块
        "pip" -> {
            drawRoundRect(
                color = color,
                topLeft = Offset(2.8f, 4.4f),
                size = Size(14.6f, 12.4f),
                cornerRadius = CornerRadius(2.2f),
                style = stroke,
            )
            // 左上箭头
            val arrow = Path().apply {
                moveTo(6.2f, 6.6f)
                lineTo(10.6f, 11.0f)
                lineTo(8.6f, 11.0f)
                lineTo(8.6f, 13.0f)
                lineTo(6.2f, 13.0f)
                lineTo(6.2f, 10.6f)
                lineTo(4.2f, 10.6f)
                close()
            }
            drawPath(arrow, color)
            // 右下实心小方块
            drawRoundRect(
                color = color,
                topLeft = Offset(13.6f, 13.4f),
                size = Size(8.0f, 7.2f),
                cornerRadius = CornerRadius(2.0f),
            )
        }

        // ---- 弹幕（开口的圆 + 汉字"弹"由调用方用 Text 叠加）----
        // 这里只画开口圆环；汉字在 [PlayerGlyph] 里用 TextMeasurer 画。
        "danmaku" -> {
            drawArc(
                color = color,
                startAngle = -52f,
                sweepAngle = 304f,
                useCenter = false,
                topLeft = Offset(3.4f, 3.4f),
                size = Size(17.2f, 17.2f),
                style = stroke,
            )
        }

        else -> drawCircle(
            color = color,
            radius = 8.4f,
            center = Offset(12f, 12f),
            style = stroke,
        )
    }
}

/** 供 [PlayerGlyph] 复用的直角矩形工具（避免每处都写 CornerRadius）。 */
internal fun DrawScope.strokeRect(
    color: Color,
    left: Float,
    top: Float,
    right: Float,
    bottom: Float,
    radius: Float,
    stroke: Stroke,
) {
    drawRoundRect(
        color = color,
        topLeft = Offset(left, top),
        size = Size(right - left, bottom - top),
        cornerRadius = CornerRadius(radius),
        style = stroke,
    )
}
