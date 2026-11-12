package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.sp

/**
 * 播放页的**统一图标族**（player-ui-v2.md 边界规则 P2）。
 *
 * ## 分工
 * · 基础/传输类（关闭、播放、暂停、下一集、快进快退、勾选）→ [drawCoreGlyph]
 * · 功能入口类（画质、均衡器、字幕、音轨、投屏、画中画、弹幕）→ [drawFeatureGlyph]
 * · 界面零件类（主页、电池、chevron、−/+、相机、重置）→ [drawChromeGlyph]
 * · 本文件只做**分发**与"弹幕"那种"图形 + 文字"的组合。
 *
 * ## 形状语言（与 REX 一致）
 * 线框为主 + 少量实心局部；线宽 1.9/24（相对坐标）。
 * 自己另创符号（比如把画质画成"方框+竖线"）会让一排图标看起来不成体系 ——
 * 用户已经明确指出这一点。
 *
 * 坐标系：以 24×24 为基准画布，外部按目标尺寸等比缩放。
 */
@Composable
internal fun PlayerGlyph(
    kind: String,
    color: Color,
    modifier: Modifier = Modifier,
    strokeWidth: Float = 1.9f,
    fill: Float = 1f,
) {
    BoxWithConstraints(modifier) {
        // 弹幕图标是"开口圆 + 汉字"，汉字要用文字排版 ⇒ 需要知道图标的实际像素尺寸。
        val density = LocalDensity.current
        val glyphPx = with(density) { maxWidth.toPx() }
        val unit = if (glyphPx > 0f) glyphPx / 24f else 0f

        Canvas(Modifier.fillMaxSize()) {
            scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
                // 逐图标线宽（REX 掩码实测，24 画布）：默认 1.9；
                // ✕ 2pt@20dp 盒 ⇒ 2.4；主页 1.3pt@22dp ⇒ 1.5；
                // 投屏/PiP 1.7pt@23dp ⇒ 1.75。其余维持族默认。
                val w = when (kind) {
                    "close" -> 2.4f
                    "home" -> 1.5f
                    "cast", "pip" -> 1.75f
                    else -> strokeWidth
                }
                val stroke = Stroke(w, cap = StrokeCap.Round)
                when {
                    kind in PANEL_GLYPHS -> drawPanelGlyph(kind, color, stroke)
                    kind in CHROME_GLYPHS -> drawChromeGlyph(kind, color, stroke, fill)
                    kind in FEATURE_GLYPHS -> drawFeatureGlyph(kind, color, stroke)
                    else -> drawCoreGlyph(kind, color, stroke)
                }
            }
        }

        if (kind == "danmaku" && unit > 0f) {
            // 汉字"弹"：用文字而不是路径（路径画汉字不现实，且字体字形更好看）。
            Text(
                text = "弹",
                color = color,
                fontSize = with(density) { (9.6f * unit).toSp() },
                maxLines = 1,
                modifier = Modifier.align(Alignment.Center),
            )
        }
    }
}

/** 属于"功能入口"一族的图标 key（走 [drawFeatureGlyph]）。 */
private val FEATURE_GLYPHS = setOf(
    "clapper",      // 跳过片段（场记板）
    "expand",       // 画面比例（圈内双向箭头）
    "audio",        // 音轨（圈内波形条，0162 锚定）
    "captions",     // 字幕（对话气泡）
    "audioMode",    // 听视频（耳机，0172 白圆亮）
    "cast",         // 投屏（AirPlay 形符号，REX 0160 掩码 1:1；协议层 DLNA）
    "pip",          // 画中画
    "danmaku",      // 弹幕（开口圆 + 弹）
)
