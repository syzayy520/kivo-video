package com.kivo.cinema.ui.detail.actions

import android.os.Build
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.updateTransition
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.TransformOrigin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.NavigationBackdrop
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import kotlin.math.roundToInt

/**
 * 255 #31「更多」毛玻璃弹出菜单（**不是** TMDB/重新加载）：
 * 添加到日历 / 下载 / 追剧进度。锚在「…」钮下方、右对齐，空间不足上翻；
 * 真背景模糊复用导航捕获桥，API31 以下白底加厚兜底。进出场 scale+fade，原点右上。
 */
@Composable
internal fun DetailMorePopup(
    open: Boolean,
    anchor: Rect,
    backdrop: NavigationBackdropState,
    onDismiss: () -> Unit,
    onAddCalendar: () -> Unit,
    onDownload: () -> Unit,
    onProgress: () -> Unit,
) {
    val transition = updateTransition(open, label = "morePopup")
    val fade by transition.animateFloat(
        transitionSpec = { tween(if (targetState) 150 else 120) }, label = "alpha") { if (it) 1f else 0f }
    val pop by transition.animateFloat(
        transitionSpec = { if (targetState) spring(dampingRatio = .72f, stiffness = 420f) else tween(120) },
        label = "scale") { if (it) 1f else .86f }
    if (!transition.currentState && !transition.targetState) return
    val shape = RoundedCornerShape(18.dp)
    val cardHDp = 166
    Box(Modifier.fillMaxSize()) {
        Box(Modifier.fillMaxSize().clickable(
            interactionSource = remember { MutableInteractionSource() },
            indication = null) { onDismiss() })
        BoxWithConstraints {
            val density = LocalDensity.current
            val gapPx = with(density) { 8.dp.toPx() }
            val cardW = with(density) { 210.dp.toPx() }
            val cardH = with(density) { cardHDp.dp.toPx() }
            val x = (anchor.right - cardW).coerceAtLeast(gapPx)
            val below = anchor.bottom + gapPx
            val flip = below + cardH > constraints.maxHeight - gapPx
            val y = if (flip) (anchor.top - gapPx - cardH).coerceAtLeast(gapPx) else below
            Box(
                Modifier.offset { IntOffset(x.roundToInt(), y.roundToInt()) }
                    .width(210.dp).height(cardHDp.dp)
                    .graphicsLayer {
                        alpha = fade; scaleX = pop; scaleY = pop
                        transformOrigin = TransformOrigin(1f, if (flip) 1f else 0f)
                    }
                    .shadow(24.dp, shape, clip = false)
                    .clip(shape)
                    .drawWithContent {
                        drawContent()
                        val corner = CornerRadius(18.dp.toPx())
                        drawRoundRect(
                            brush = Brush.verticalGradient(
                                0f to Color.White.copy(alpha = .28f), 1f to Color.Transparent,
                                startY = 0f, endY = size.height * .5f),
                            size = size, cornerRadius = corner)
                        drawRoundRect(color = Color.White.copy(alpha = .35f), size = size,
                            cornerRadius = corner, style = Stroke(width = 1.dp.toPx()))
                    },
            ) {
                val blurOk = Build.VERSION.SDK_INT >= 31
                if (blurOk) NavigationBackdrop(backdrop, Modifier.fillMaxSize(), blurRadius = 48f)
                Box(Modifier.fillMaxSize()
                    .background(Color.White.copy(alpha = if (blurOk) .68f else .94f)))
                Column(Modifier.fillMaxWidth().padding(vertical = 8.dp)) {
                    MoreRow(ActionGlyph.Calendar, "添加到日历", onAddCalendar)
                    MoreRow(ActionGlyph.Download, "下载", onDownload)
                    MoreRow(ActionGlyph.Progress, "追剧进度", onProgress)
                }
            }
        }
    }
}

@Composable
private fun MoreRow(glyph: ActionGlyph, title: String, onClick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    Row(
        Modifier.fillMaxWidth().height(50.dp).padding(horizontal = 6.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(if (pressed) Color.Black.copy(alpha = .06f) else Color.Transparent)
            .clickable(interaction, indication = null) { onClick() }
            .padding(start = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        DetailActionGlyph(glyph, DesignTokens.textMain, Modifier.size(20.dp))
        Spacer(Modifier.width(12.dp))
        Text(title, fontSize = 16.sp, fontWeight = FontWeight.Medium, color = DesignTokens.textMain)
    }
}