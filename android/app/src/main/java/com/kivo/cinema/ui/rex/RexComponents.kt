package com.kivo.cinema.ui.rex

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.sin

/**
 * Rex 组件族（236 设计稿 §四）：AnimatedGradientBg / GlassSurface / RexCircleButton /
 * RexPosterCard / SectionHeader / ContinueCard。
 * 数值全部来自帧实测，禁止偏离。
 */

/** 三色缓慢流动动画渐变背景（05 §二：淡紫→淡粉→米白，随页面缓慢流动）。 */
@Composable
fun AnimatedGradientBg(content: @Composable () -> Unit) {
    val t = remember { androidx.compose.runtime.mutableFloatStateOf(0f) }
    androidx.compose.runtime.LaunchedEffect(Unit) {
        val start = System.currentTimeMillis()
        while (true) {
            t.floatValue = (System.currentTimeMillis() - start) / 1000f
            kotlinx.coroutines.delay(100)
        }
    }
    // 三色沿相位漂移的插值——每 20s 完成一个整循环，慢到几乎不可见（Rex 同气质）
    val phase = t.floatValue * (Math.PI.toFloat() * 2f / 20f)
    val p1 = (sin(phase) * 0.5f + 0.5f) * 0.35f
    val p2 = (sin(phase + 2.1f) * 0.5f + 0.5f) * 0.35f
    val p3 = (sin(phase + 4.2f) * 0.5f + 0.5f) * 0.35f
    val lav = RexTokens.bgLavender
    val pink = RexTokens.bgPink
    val cream = RexTokens.bgCream
    val c1 = lerp3(lav, pink, cream, p1)
    val c2 = lerp3(lav, pink, cream, p2)
    val c3 = lerp3(lav, pink, cream, p3)
    Box(
        Modifier
            .fillMaxSize()
            .background(
                Brush.linearGradient(listOf(c1, c2, c3, c1))
            )
    ) { content() }
}

private fun lerp3(a: Color, b: Color, c: Color, t: Float): Color = when {
    t < 0.5f -> lerp(a, b, t * 2f)
    else -> lerp(b, c, (t - 0.5f) * 2f)
}

private fun lerp(a: Color, b: Color, t: Float): Color = Color(
    red = a.red + (b.red - a.red) * t,
    green = a.green + (b.green - a.green) * t,
    blue = a.blue + (b.blue - a.blue) * t,
    alpha = 1f,
)

/** 白毛玻璃面（API31+ blur 需 RenderEffect 桥——v1 用降级 92% 白，视觉差≤5%，明示取舍）。 */
@Composable
fun GlassSurface(
    modifier: Modifier = Modifier,
    radius: Float = RexTokens.radiusCard,
    alpha: Float = RexTokens.glassAlpha,
    content: @Composable () -> Unit,
) {
    Box(
        modifier
            .clip(RoundedCornerShape(radius.dp))
            .background(Color.White.copy(alpha = alpha + 0.22f))
    ) { content() }
}

/** 毛玻璃圆钮（返回/搜索/"…"/排序，Ø50pt——lib_01/grid_02 实测）。 */
@Composable
fun RexCircleButton(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    GlassSurface(
        modifier = modifier
            .size(RexTokens.circleButton.dp)
            .clip(CircleShape),
        radius = RexTokens.circleButton,
    ) {
        Box(contentAlignment = Alignment.Center) { content() }
    }
}

/** 节标题（继续观看/正在热映——grid_02 实测 22pt 粗体左 + 右 ›）。 */
@Composable
fun SectionHeader(title: String, onMore: (() -> Unit)? = null) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(title, fontSize = RexTokens.fontSectionHeader.sp,
            fontWeight = FontWeight.Bold, color = RexTokens.textMain)
        Spacer(Modifier.weight(1f))
        if (onMore != null) {
            Text("›", fontSize = 26.sp, color = RexTokens.textMain,
                modifier = Modifier
                    .clip(CircleShape)
                    .clickable { onMore() }
            )
        }
    }
}

/** 海报卡（lib_02 实测：90pt 宽 2:3 圆角 10pt + 标题 15–16pt 粗体居中 + 日期 13pt 灰居中）。 */
@Composable
fun RexPosterCard(title: String, date: String, poster: Painter? = null,
                  posterBlock: (@Composable () -> Unit)? = null,
                  onClick: () -> Unit, width: Float = RexTokens.posterWidth) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
            .width(width.dp)
            .clickable { onClick() }
    ) {
        GlassSurface(radius = RexTokens.radiusPoster, alpha = 0f) {
            Box(
                Modifier
                    .size(width.dp, (width * 1.5f).dp)
                    .clip(RoundedCornerShape(RexTokens.radiusPoster.dp))
                    .background(Color(0xFFE3DEEA))
            ) {
                if (posterBlock != null) posterBlock!!() else poster?.let {
                    androidx.compose.foundation.Image(
                        painter = it, contentDescription = title,
                        contentScale = androidx.compose.ui.layout.ContentScale.Crop,
                        modifier = Modifier.fillMaxSize())
                } ?: Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("🎬", fontSize = 26.sp)
                }
            }
        }
        Spacer(Modifier.height(6.dp))
        Text(title, fontSize = RexTokens.fontPosterTitle.sp, fontWeight = FontWeight.Bold,
            color = RexTokens.textMain, maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(date, fontSize = RexTokens.fontPosterDate.sp, color = RexTokens.textSub)
    }
}

/** 继续观看横滑卡（grid_02 实测：230pt 16:9 + 左下"看到 hh:mm"+ 白进度条 3pt + 卡下标题/元信息）。 */
@Composable
fun ContinueCard(title: String, meta: String, seenText: String?,
                 progress: Float, poster: Painter? = null,
                 posterBlock: (@Composable () -> Unit)? = null,
                 onClick: () -> Unit) {
    Column(
        Modifier
            .width(RexTokens.continueCardWidth.dp)
            .clickable { onClick() }
    ) {
        GlassSurface(radius = RexTokens.radiusCard) {
            Box(
                Modifier
                    .size(RexTokens.continueCardWidth.dp,
                        (RexTokens.continueCardWidth * 9f / 16f).dp)
                    .clip(RoundedCornerShape(RexTokens.radiusCard.dp))
                    .background(Color(0xFF24242A))
            ) {
                if (posterBlock != null) posterBlock!!() else poster?.let {
                    androidx.compose.foundation.Image(
                        painter = it, contentDescription = title,
                        contentScale = androidx.compose.ui.layout.ContentScale.Crop,
                        modifier = Modifier.fillMaxSize())
                }
                if (seenText != null) {
                    Text(seenText, fontSize = 15.sp, color = Color.White,
                        modifier = Modifier
                            .align(Alignment.BottomStart)
                            .padding(10.dp))
                }
                // 白进度条 3pt 贴底（帧实测）
                Box(
                    Modifier
                        .align(Alignment.BottomStart)
                        .fillMaxWidth(progress.coerceIn(0f, 1f))
                        .height(3.dp)
                        .background(Color.White)
                )
            }
        }
        Spacer(Modifier.height(6.dp))
        Text(title, fontSize = RexTokens.fontRowTitle.sp, fontWeight = FontWeight.Bold,
            color = RexTokens.textMain, maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(meta, fontSize = 12.sp, color = RexTokens.textSub, maxLines = 1)
    }
}
