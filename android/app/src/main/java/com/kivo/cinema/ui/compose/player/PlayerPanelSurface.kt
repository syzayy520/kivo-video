package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.drawWithCache
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.Outline
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.drawscope.clipPath
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.toIntSize

/**
 * **面板材质**的唯一 Modifier（P10 §3；v3 = SenPlayer 对齐的浅色玻璃，spec §16）。
 *
 * v3 为了"观感对齐 SenPlayer"，把 v2 的暗色板整体翻成浅色玻璃：
 * · 底：白 40% → 37%（对方实测反解 ≈40%；深底 30 上合成 ≈120）；
 * · 受光：白 8% 从顶边衰减到面板高 45%（浅底上必须比 v2 的 6% 更亮）；
 * · 边缘：整圈 1dp 白 14% 内描边（用户选「弱描边」）＋ 顶边 1dp 白 32%（左右 24dp 渐隐）
 *   ＋ 底缘 1dp 黑 13%（浅底不宜压黑）；
 * · 圆角 13.5dp（对方实测 36–38px @2.75）；阴影 24dp 黑 35%。
 * ⚠️ 对比度判据已按用户 2026-11-21 拍板撤销（白字压中灰 ≈2.1:1，与对方同量级），见 spec §16.1。
 *
 * 【为什么不用模糊】视频层是 SurfaceView，独立合成，Compose 拿不到它的像素；
 * `Modifier.blur` / `RenderEffect` 只作用于自身 layer ⇒ 做不出"模糊背后的视频"。
 * 所以层次靠"底色 + 受光 + 边缘 + 投影"四件事，而不是靠加黑。
 *
 * 【为什么 Toast 也走这里】同一个材质族；Toast 只是参数不同（半径 14 / 阴影 8 / 单色底 /
 * 无受光 / 无页眉带）——共用一段绘制代码，材质才不会分叉成两套。
 *
 * @param headerHeight 有头部时传头部高度，顶部会叠一条 [PlayerPanelStyle.headerWash] 页眉带；
 *   传 0 表示无头部（Compact 列表、Toast）。
 * @param backdrop 可选的抓帧模糊图（[PanelBackdrop] 产出），铺满面板、裁在圆角内，并叠
 *   传 null（默认）时整层跳过，绘制与不带背景图时逐像素一致。
 */
internal fun Modifier.playerPanelSurface(
    radius: Dp = PlayerPanelStyle.radius,
    elevation: Dp = PlayerPanelStyle.shadow,
    top: Color = PlayerPanelStyle.surfaceTop,
    bottom: Color = PlayerPanelStyle.surfaceBottom,
    sheen: Color = PlayerPanelStyle.topSheen,
    edge: Color = PlayerPanelStyle.edgeLine,
    edgeTop: Color = PlayerPanelStyle.edgeTop,
    edgeBottom: Color = PlayerPanelStyle.edgeBottom,
    edgeFade: Dp = PlayerPanelStyle.edgeFade,
    headerWash: Color = Color.Transparent,
    headerHeight: Dp = 0.dp,
    shape: Shape = RoundedCornerShape(radius),
    backdrop: ImageBitmap? = null,
): Modifier = this
    .shadow(
        elevation = elevation,
        shape = shape,
        ambientColor = PlayerPanelStyle.shadowColor,
        spotColor = PlayerPanelStyle.shadowColor,
        clip = false,
    )
    .drawWithCache {
        val path = shape.toPanelPath(size, layoutDirection, this)
        val base = Brush.verticalGradient(
            colors = listOf(top, bottom),
            startY = 0f,
            endY = size.height,
        )
        // 受光面：顶边到面板高 45% 渐变，超出部分按 Clamp 取末端 Transparent。
        val highlight = if (sheen.alpha > 0f) Brush.verticalGradient(
            colors = listOf(sheen, Color.Transparent),
            startY = 0f,
            endY = size.height * PlayerPanelStyle.sheenRatio,
        ) else null
        // 顶边受光线：横向渐隐；裁到圆角路径内，转角自然收掉，不出现方角亮线。
        val edgeBrush = if (edgeTop.alpha > 0f && size.width > 0f) {
            val fade = (edgeFade.toPx() / size.width).coerceIn(0f, 0.5f)
            Brush.horizontalGradient(
                colorStops = arrayOf(
                    0f to Color.Transparent,
                    fade to edgeTop,
                    (1f - fade) to edgeTop,
                    1f to Color.Transparent,
                ),
                startX = 0f,
                endX = size.width,
            )
        } else null
        val hairline = 1.dp.toPx()
        val headerPx = headerHeight.toPx()
        // 页眉带做成"上亮下透"的渐变（不是一条硬边矩形）：头部高度处平滑收掉，
        // 滚动/无头部的面板都不会看到一条横线。
        val headerBrush = if (headerPx > 0f && headerWash.alpha > 0f) Brush.verticalGradient(
            colors = listOf(headerWash, Color.Transparent),
            startY = 0f,
            endY = headerPx,
        ) else null
        onDrawBehind {
            // 背景图在渐变底之下、圆角之内：贴图后叠 35% 白，画面只剩"色块"，
            // 后面的受光/边缘/页眉带顺序不变，仍压在它上面。
            if (backdrop != null) {
                clipPath(path) {
                    drawImage(
                        image = backdrop,
                        dstOffset = IntOffset.Zero,
                        dstSize = size.toIntSize(),
                    )
                    // 不再额外叠白洗：两层白（此处 35% + 玻璃 40%）会把亮场推到 241，
                    // 比 SenPlayer 更亮、白字更糊。只留玻璃那一层后与对方公式等价（0.4·255+0.6·L）。
                }
            }
            drawPath(path, brush = base)
            if (highlight != null) drawPath(path, brush = highlight)
            clipPath(path) {
                if (headerBrush != null) {
                    drawRect(
                        brush = headerBrush,
                        topLeft = Offset.Zero,
                        size = Size(size.width, headerPx),
                    )
                }
                if (edgeBrush != null) {
                    drawRect(
                        brush = edgeBrush,
                        topLeft = Offset.Zero,
                        size = Size(size.width, hairline),
                    )
                }
                if (edgeBottom.alpha > 0f) {
                    drawRect(
                        color = edgeBottom,
                        topLeft = Offset(0f, size.height - hairline),
                        size = Size(size.width, hairline),
                    )
                }
                // 整圈内描边：Stroke 以路径为中心线，外侧一半被 clipPath 裁掉，
                // 所以宽度取 2dp 才能得到可见 1dp；圆角处同样只留内侧，自然收口。
                if (edge.alpha > 0f) {
                    drawPath(
                        path = path,
                        color = edge,
                        style = androidx.compose.ui.graphics.drawscope.Stroke(2 * hairline),
                    )
                }
            }
        }
    }


/**
 * **选中行底**：品牌绿 7%、10dp 圆角、左右各向外让 8dp。
 *
 * 【为什么要这块底】v1 的"当前轨"只有行尾一个勾 —— 滚到列表中间时眼睛要先扫到行尾
 * 才知道哪条是当前值。加一层 7% 绿底后，选中行在暗底上是一整块可扫的色块，
 * 勾退回"确认"的辅助角色（不是唯一线索）。
 *
 * 【几何为什么向左多让 8dp】行内容左缘在面板内 20dp 处；底色左移 8dp 后距面板边 12dp，
 * 与右缘（距边 8dp + 行尾留白 16dp）视觉对称，不会顶到圆角上。
 */
internal fun Modifier.panelRowWash(selected: Boolean): Modifier =
    if (!selected) this else drawBehind {
        val inset = PlayerPanelStyle.rowWashInset.toPx()
        drawRoundRect(
            color = PlayerPanelStyle.selectedRowWash,
            topLeft = Offset(-inset, 0f),
            size = size,
            cornerRadius = CornerRadius(PlayerPanelStyle.rowWashCorner.toPx()),
        )
    }

/** Shape → Path：三种 Outline 各自转一次，避免依赖版本相关的 `drawOutline(brush)` 重载。 */
private fun Shape.toPanelPath(size: Size, layoutDirection: LayoutDirection, density: Density): Path {
    val outline = createOutline(size, layoutDirection, density)
    return Path().apply {
        when (outline) {
            is Outline.Rectangle -> addRect(outline.rect)
            is Outline.Rounded -> addRoundRect(outline.roundRect)
            is Outline.Generic -> addPath(outline.path)
        }
    }
}