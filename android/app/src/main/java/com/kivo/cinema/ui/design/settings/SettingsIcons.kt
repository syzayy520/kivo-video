package com.kivo.cinema.ui.design.settings

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.graphics.vector.PathParser
import androidx.compose.ui.unit.dp

/**
 * 设置页线 glyph 集（268）：白色描边 ImageVector，渲染时用 ColorFilter.tint 上域色。
 * 只负责"glyph 形状"，chip 淡底/尺寸在 SettingsRowParts，色板在 SettingsVisuals。
 * 24 viewport / 1.9 描边 / 圆头圆角，与草图 v2 一致。
 */
internal object SettingsIcons {
    private fun glyph(vararg paths: String): ImageVector =
        ImageVector.Builder(
            defaultWidth = 24.dp, defaultHeight = 24.dp,
            viewportWidth = 24f, viewportHeight = 24f,
        ).apply {
            paths.forEach {
                addPath(
                    pathData = PathParser().parsePathString(it).toNodes(),
                    stroke = SolidColor(Color.White),
                    strokeLineWidth = 1.9f,
                    strokeLineCap = StrokeCap.Round,
                    strokeLineJoin = StrokeJoin.Round,
                )
            }
        }.build()

    val play = glyph("M8 5v14l11-7z")
    val sliders = glyph("M4 10v4", "M8 7v10", "M12 4v16", "M16 7v10", "M20 10v4")
    val hand = glyph("M9 11V6a2 2 0 1 1 4 0v5", "M13 10l4 1 2 4-3 6H9l-3-5 3-2")
    val subtitle = glyph("M3 5h18v14H3z", "M7 15h4", "M13 15h4")
    val drive = glyph("M3 6h18v12H3z", "M7 15h.01", "M11 15h6")
    val brush = glyph("M14 4l6 6-9 9-4 1 1-4z", "M12 6l6 6")
    val lock = glyph("M5 10h14v10H5z", "M8 10V7a4 4 0 0 1 8 0v3")
    val info = glyph("M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18z", "M12 11v5", "M12 8h.01")
    val cloud = glyph("M7 18a4 4 0 0 1 0-8 5 5 0 0 1 9.6-1.4A4 4 0 0 1 17 18z")
    val globe = glyph("M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18z", "M3 12h18", "M12 3a14 14 0 0 1 0 18")
    val image = glyph("M4 4h16v16H4z", "M4 14l4-3 3 2 4-4 5 5")
    val menu = glyph("M4 6h16", "M4 12h16", "M4 18h10")
    val speaker = glyph("M11 5L6 9H3v6h3l5 4z", "M15 9a4 4 0 0 1 0 6")
    val theme = glyph("M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18z", "M12 3a9 9 0 0 1 0 18z")
    val film = glyph("M3 5h18v14H3z", "M3 9h18", "M7 5v4", "M12 5v4", "M17 5v4")
    val download = glyph("M12 4v10", "M8 10l4 4 4-4", "M4 19h16")
    val shield = glyph("M12 3l8 3v6c0 5-4 8-8 9-4-1-8-4-8-9V6z")
}