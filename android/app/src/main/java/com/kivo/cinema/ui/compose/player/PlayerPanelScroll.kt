package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.ScrollState
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshotFlow
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithCache
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.unit.Dp
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlin.math.max

/**
 * 面板滚动的三件小事（P10 §9）：**边缘渐隐 / 自动隐藏指示条 / 内容淡入**。
 *
 * 拆成独立文件的理由与 [PlayerGlassMenuParts] 相同：它们都是"滚动这件行为"的零件，
 * 谁都不决定面板布局 —— 布局留在壳里，零件可单独替换。
 */

/**
 * 上下边缘 16dp 渐隐。
 *
 * 【为什么必须离屏 layer + DstOut】直接盖一层黑渐变会把"透出面板底"变成"透出黑"， 
 * 在浅色画面上就是一坨脏污；`DstOut` 只削掉**内容自己的 alpha**，
 * 面板底（由 [playerPanelSurface] 画在更外层）原样透出来。
 *
 * 用法上必须挂在 `verticalScroll` **之前**（外层）：挂在里面就会跟着内容一起滚，
 * 渐隐边跟着走，等于没做。
 */
internal fun Modifier.panelVerticalFade(
    top: Boolean,
    bottom: Boolean,
    fade: Dp = PlayerPanelStyle.scrollFade,
    fadeBottom: Dp = fade,
): Modifier = if (!top && !bottom) this else drawWithCache {
    val topPx = fade.toPx()
    val bottomPx = fadeBottom.toPx()
    val h = size.height
    // 渐变在缓存阶段构建一次，绘制期不再每帧 new Brush。
    val topBrush = if (top && topPx > 0f) Brush.verticalGradient(
        colors = listOf(Color.Black, Color.Transparent),
        startY = 0f,
        endY = topPx,
    ) else null
    val bottomBrush = if (bottom && bottomPx > 0f) Brush.verticalGradient(
        colors = listOf(Color.Transparent, Color.Black),
        startY = h - bottomPx,
        endY = h,
    ) else null
    onDrawWithContent {
        if (h <= 0f) {
            drawContent()
            return@onDrawWithContent
        }
        drawContext.canvas.saveLayer(Rect(0f, 0f, size.width, h), Paint())
        drawContent()
        if (topBrush != null) {
            drawRect(brush = topBrush, blendMode = BlendMode.DstOut)
        }
        if (bottomBrush != null) {
            drawRect(brush = bottomBrush, blendMode = BlendMode.DstOut)
        }
        drawContext.canvas.restore()
    }
}

/**
 * 滚动指示条（§9）：2dp 宽、右内 4dp、拇指 `max(28dp, 可视比例×视口)`、白 30% 圆头；
 * 只在滚动中及**停手后 600ms** 显示，进出 120/300ms 线性 alpha。
 *
 * 【为什么不用 `VerticalScrollbar`】Material 的滚动条是另一套语言（圆角/颜色/出现时机），
 * 放在 Apple 风面板里会立刻看出"不是同一个 app"。
 */
@Composable
internal fun PanelScrollbar(
    state: ScrollState,
    modifier: Modifier = Modifier,
) {
    var active by remember { mutableStateOf(false) }
    // 只把 state 放进 key：滚动期间 key 不变 ⇒ 协程不重启，监听靠 snapshotFlow 取值。
    LaunchedEffect(state) {
        var hideJob: Job? = null
        snapshotFlow { state.isScrollInProgress }.collect { scrolling ->
            hideJob?.cancel()
            hideJob = null
            if (scrolling) {
                active = true
            } else {
                hideJob = launch {
                    delay(PlayerPanelStyle.scrollbarHoldMs)
                    active = false
                }
            }
        }
    }
    val alpha by animateFloatAsState(
        targetValue = if (active) 1f else 0f,
        animationSpec = tween(
            if (active) PlayerPanelStyle.motion.scrollbarInMs else PlayerPanelStyle.motion.scrollbarOutMs,
            easing = LinearEasing,
        ),
        label = "panelScrollbar",
    )
    if (state.maxValue <= 0 || alpha <= 0f) return
    Canvas(
        modifier = modifier
            .padding(end = PlayerPanelStyle.scrollbarInset)
            .width(PlayerPanelStyle.scrollbarWidth)
            .fillMaxHeight(),
    ) {
        val viewport = state.viewportSize.toFloat()
        val total = viewport + state.maxValue
        if (viewport <= 0f || total <= 0f) return@Canvas
        val thumb = max(PlayerPanelStyle.scrollbarThumbMin.toPx(), size.height * viewport / total)
            .coerceAtMost(size.height)
        val maxScroll = state.maxValue.toFloat()
        val fraction = if (maxScroll > 0f) (state.value / maxScroll).coerceIn(0f, 1f) else 0f
        drawRoundRect(
            color = PlayerPanelStyle.scrollbarThumb.copy(
                alpha = PlayerPanelStyle.scrollbarThumb.alpha * alpha,
            ),
            topLeft = Offset(0f, (size.height - thumb) * fraction),
            size = Size(size.width, thumb),
            cornerRadius = CornerRadius(size.width / 2f),
        )
    }
}

/** 内容空→有 120ms 线性淡入（§8：不做行错峰、不做假 blur）。 */
@Composable
internal fun panelContentAlpha(hasContent: Boolean, reduceMotion: Boolean): Float {
    val alpha by animateFloatAsState(
        targetValue = if (hasContent) 1f else 0f,
        animationSpec = tween(
            if (reduceMotion) PlayerPanelStyle.motion.reduceMs else PlayerPanelStyle.motion.contentFadeMs,
            easing = LinearEasing,
        ),
        label = "panelContent",
    )
    return alpha
}