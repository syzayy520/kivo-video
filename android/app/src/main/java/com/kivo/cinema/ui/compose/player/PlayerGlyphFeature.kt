package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
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
 * | 圈内波形条 | 均衡器 | 圆 + 5 根高低不等的竖条 |
 * | 对话气泡 | 字幕 | 圆角气泡（左下小尾巴）+ 两行横线 |
 * | 耳机 | 音轨 | 圆弧头梁 + 两只耳罩 |
 * | AirPlay | 投屏 | 屏幕圆角框 + 实心三角 |
 * | 画中画 | PiP | 屏幕框 + 左上斜箭头 + 右下实心小方块 |
 * | 弹（圈内） | 弹幕 | 开口的圆 + 汉字"弹" |
 *
 * 【画法】全部是**线框 + 实心局部**的混搭，与 REX 一致；
 * 坐标系 24×24，与 REX 的符号比例接近。
 */
internal fun DrawScope.drawFeatureGlyph(kind: String, color: Color, stroke: Stroke) {
    when (kind) {
        // ---- 场记板（画质比例）----
        // 顶板斜着上翘 + 4 道斜条纹 + 下方圆角机身
        "clapper" -> {
            rotate(degrees = -11f, pivot = Offset(12f, 9f)) {
                drawRoundRect(
                    color = color,
                    topLeft = Offset(3.2f, 4.6f),
                    size = Size(17.6f, 5.2f),
                    cornerRadius = CornerRadius(1.2f),
                    style = stroke,
                )
                // 条纹：4 道斜杠
                var x = 6.2f
                while (x < 19f) {
                    drawLine(
                        color = color,
                        start = Offset(x, 5.1f),
                        end = Offset(x - 2.2f, 9.3f),
                        strokeWidth = stroke.width,
                        cap = StrokeCap.Round,
                    )
                    x += 3.6f
                }
            }
            // 机身
            drawRoundRect(
                color = color,
                topLeft = Offset(3.2f, 10.4f),
                size = Size(17.6f, 10.2f),
                cornerRadius = CornerRadius(1.8f),
                style = stroke,
            )
        }

        // ---- 圈内双向箭头（画面缩放）----
        // 圆 + 左上一支朝左上的箭头 + 右下一支朝右下的箭头（带杆，不是小三角）
        "expand" -> {
            drawCircle(
                color = color,
                radius = 9.4f,
                center = Offset(12f, 12f),
                style = stroke,
            )
            // 左上箭头
            drawLine(color, Offset(10.5f, 9.6f), Offset(7.5f, 6.6f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(7.5f, 6.6f), Offset(9.8f, 7.0f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(7.5f, 6.6f), Offset(7.1f, 8.9f), stroke.width, StrokeCap.Round)
            // 右下箭头
            drawLine(color, Offset(13.5f, 14.4f), Offset(16.5f, 17.4f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(16.5f, 17.4f), Offset(14.2f, 17.0f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(16.5f, 17.4f), Offset(16.9f, 15.1f), stroke.width, StrokeCap.Round)
        }

        // ---- 圈内波形条（均衡器）----
        // 圆 + 5 根高低不等的竖条（中间最高）
        "equalizer" -> {
            drawCircle(
                color = color,
                radius = 9.4f,
                center = Offset(12f, 12f),
                style = stroke,
            )
            val heights = listOf(4.2f, 7.4f, 10.2f, 7.0f, 4.0f)
            var i = 0
            while (i < heights.size) {
                val x = 7.6f + i * 2.2f
                val h = heights[i]
                drawLine(
                    color = color,
                    start = Offset(x, 12f - h / 2f),
                    end = Offset(x, 12f + h / 2f),
                    strokeWidth = stroke.width * 1.15f,
                    cap = StrokeCap.Round,
                )
                i++
            }
        }

        // ---- 对话气泡（字幕）----
        // 圆角矩形 + 左下小尾巴 + 两行横线（上行长、下行两段短）
        "captions" -> {
            val bubble = Path().apply {
                moveTo(4.4f, 5.2f)
                lineTo(19.6f, 5.2f)
                quadraticTo(21.2f, 5.2f, 21.2f, 6.8f)
                lineTo(21.2f, 15.4f)
                quadraticTo(21.2f, 17.0f, 19.6f, 17.0f)
                lineTo(10.2f, 17.0f)
                lineTo(7.4f, 20.2f)   // 尾巴
                lineTo(7.4f, 17.0f)
                lineTo(4.4f, 17.0f)
                quadraticTo(2.8f, 17.0f, 2.8f, 15.4f)
                lineTo(2.8f, 6.8f)
                quadraticTo(2.8f, 5.2f, 4.4f, 5.2f)
                close()
            }
            drawPath(bubble, color, style = stroke)
            // 两行横线
            drawLine(color, Offset(6.4f, 9.4f), Offset(17.6f, 9.4f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(6.4f, 12.6f), Offset(12.6f, 12.6f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(14.6f, 12.6f), Offset(17.6f, 12.6f), stroke.width, StrokeCap.Round)
        }

        // ---- 耳机（音轨）----
        // 圆弧头梁 + 两只实心耳罩
        "headphones" -> {
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

        // ---- AirPlay（投屏）----
        // 屏幕圆角框（缺口在底部）+ 实心三角
        "airplay" -> {
            val screen = Path().apply {
                moveTo(3.2f, 4.6f)
                lineTo(20.8f, 4.6f)
                lineTo(20.8f, 15.0f)
                lineTo(15.6f, 15.0f)
                lineTo(12f, 18.2f)
                lineTo(8.4f, 15.0f)
                lineTo(3.2f, 15.0f)
                close()
            }
            drawPath(screen, color, style = stroke)
            // 实心三角（投屏符号）
            val tri = Path().apply {
                moveTo(12f, 8.0f)
                lineTo(16.4f, 13.0f)
                lineTo(7.6f, 13.0f)
                close()
            }
            drawPath(tri, color)
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

/** 未使用的占位，保持 Rect 导入被使用（避免 IDE 警告）。 */
internal val glyphUnitRect = Rect(0f, 0f, 24f, 24f)
