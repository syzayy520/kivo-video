package com.kivo.cinema.ui.detail

import android.os.Build
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.updateTransition
import androidx.compose.foundation.Canvas
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
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.TransformOrigin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.layout.positionInRoot
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.toSize
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.NavigationBackdrop
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.math.roundToInt

/**
 * 选季状态机：首季随 content 预载；切季按需加载，失败清空列表并给 reason（不静默）。
 * 状态归选季家族持有，DetailPage 只编排。
 */
internal class SeasonState(season: EmbyShows.Season?, episodes: List<EmbyShows.Episode>) {
    var season by mutableStateOf(season)
    var episodes by mutableStateOf(episodes)
    var loading by mutableStateOf(false)
    var error by mutableStateOf<String?>(null)
    internal var loadedId: String? = season?.id
    private var retryTick by mutableStateOf(0)
    fun retry() { retryTick++ }
    internal val tick: Int get() = retryTick
}

@Composable
internal fun rememberSeasonState(content: DetailContent?): SeasonState {
    val state = remember(content) {
        SeasonState(content?.seasons?.firstOrNull(), content?.episodes ?: emptyList())
    }
    LaunchedEffect(state, state.season, state.tick) {
        val s = state.season ?: return@LaunchedEffect
        val loader = content?.loadEpisodes ?: return@LaunchedEffect
        if (s.id == state.loadedId) return@LaunchedEffect
        state.loading = true; state.error = null
        try {
            state.episodes = withContext(Dispatchers.IO) { loader(s) }
            state.loadedId = s.id
        } catch (e: CancellationException) { throw e }
        catch (_: Exception) {
            state.episodes = emptyList()
            state.error = "无法读取该季剧集，请检查网络后重试"
        } finally { state.loading = false }
    }
    return state
}

/**
 * 255 选季控件（帧实测）。
 *
 * 触发器＝节标题式粗体「第 N 季」+ 自绘 ⌄（**无胶囊底**）；
 * 浮层＝白毛玻璃**真背景模糊**（缩略图在玻璃后糊成色块）、圆角 20、宽 250、
 * 行高 42、**勾在行左**（勾心 +26 / 文本左缘 +45）、无分隔线、背景不压暗。
 * 模糊复用首页导航栏的 [NavigationBackdrop] 捕获桥；API31 以下无 RenderEffect，
 * 白底加厚到 94% 兜底（与 GlassSurface 同一降级口径）。
 */
@Composable
internal fun SeasonTrigger(
    name: String,
    expandable: Boolean,
    expanded: Boolean,
    fg: Color,
    onAnchor: (Rect) -> Unit,
    onClick: () -> Unit,
) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    val press by animateFloatAsState(
        if (pressed) .96f else 1f, spring(dampingRatio = .6f, stiffness = 500f), label = "press")
    val rotation by animateFloatAsState(
        if (expanded) 180f else 0f, spring(dampingRatio = .7f, stiffness = 380f), label = "chevron")
    Row(
        Modifier
            .onGloballyPositioned { onAnchor(Rect(it.positionInRoot(), it.size.toSize())) }
            .graphicsLayer { scaleX = press; scaleY = press }
            .clickable(interaction, indication = null, enabled = expandable) { onClick() },
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(name, fontSize = DETAIL_SECTION_TITLE_SP.sp,
            fontWeight = FontWeight.Bold, color = fg)
        if (expandable) {
            Spacer(Modifier.width(10.dp))
            Canvas(Modifier.size(20.dp).graphicsLayer { rotationZ = rotation }) {
                val s = size.width / 24f
                val w = 2.4f * s
                drawLine(fg, Offset(5f * s, 9f * s), Offset(12f * s, 16f * s), w, StrokeCap.Round)
                drawLine(fg, Offset(12f * s, 16f * s), Offset(19f * s, 9f * s), w, StrokeCap.Round)
            }
        }
    }
}

/**
 * 选季浮层：透明点击兜底（不压暗背景）+ 锚在触发器下方的玻璃卡。
 * 进出场 scale+fade、原点左上（从触发器展开）；退出动画期间保持挂载。
 */
@Composable
internal fun SeasonMenuOverlay(
    expanded: Boolean,
    anchor: Rect,
    seasons: List<EmbyShows.Season>,
    current: EmbyShows.Season?,
    backdrop: NavigationBackdropState,
    onDismiss: () -> Unit,
    onPick: (EmbyShows.Season) -> Unit,
) {
    val transition = updateTransition(expanded, label = "seasonMenu")
    val fade by transition.animateFloat(
        transitionSpec = { tween(if (targetState) 150 else 120) }, label = "alpha") {
        if (it) 1f else 0f
    }
    val pop by transition.animateFloat(
        transitionSpec = {
            if (targetState) spring(dampingRatio = .72f, stiffness = 420f) else tween(120)
        }, label = "scale") { if (it) 1f else .86f }
    if (!transition.currentState && !transition.targetState) return
    val shape = RoundedCornerShape(20.dp)
    Box(Modifier.fillMaxSize()) {
        Box(Modifier.fillMaxSize().clickable(
            interactionSource = remember { MutableInteractionSource() },
            indication = null) { onDismiss() })
        BoxWithConstraints {
            val density = LocalDensity.current
            val gapPx = with(density) { 8.dp.toPx() }
            val cardHDp = (seasons.size * 42 + 20).coerceAtMost(336)
            val cardH = with(density) { cardHDp.dp.toPx() }
            val x = anchor.left
            val below = anchor.bottom + gapPx
            val flip = below + cardH > constraints.maxHeight - gapPx
            val y = if (flip) (anchor.top - gapPx - cardH).coerceAtLeast(gapPx) else below
            Box(
                Modifier
                    .offset { IntOffset(x.roundToInt(), y.roundToInt()) }
                    .width(250.dp)
                    .height(cardHDp.dp)
                    .graphicsLayer {
                        alpha = fade
                        scaleX = pop; scaleY = pop
                        transformOrigin = TransformOrigin(0f, if (flip) 1f else 0f)
                    }
                    .shadow(24.dp, shape, clip = false)
                    .clip(shape)
                    .drawWithContent {
                        drawContent()
                        val corner = CornerRadius(20.dp.toPx())
                        drawRoundRect(
                            brush = Brush.verticalGradient(
                                0f to Color.White.copy(alpha = .28f),
                                1f to Color.Transparent,
                                startY = 0f, endY = size.height * .5f),
                            size = size, cornerRadius = corner)
                        drawRoundRect(
                            color = Color.White.copy(alpha = .35f),
                            size = size, cornerRadius = corner,
                            style = Stroke(width = 1.dp.toPx()))
                    },
            ) {
                val blurOk = Build.VERSION.SDK_INT >= 31
                if (blurOk) {
                    NavigationBackdrop(backdrop, Modifier.fillMaxSize(), blurRadius = 48f)
                }
                Box(Modifier.fillMaxSize()
                    .background(Color.White.copy(alpha = if (blurOk) .68f else .94f)))
                Column(
                    Modifier.fillMaxWidth().heightIn(max = 336.dp)
                        .verticalScroll(rememberScrollState()).padding(vertical = 10.dp),
                ) {
                    seasons.forEach { s ->
                        SeasonMenuRow(s, s.id == current?.id) { onPick(s) }
                    }
                }
            }
        }
    }
}

/** 一行：左勾槽（未选中留空占位）+ 17pt 常规黑字；按压灰 wash（iOS 菜单手感）。 */
@Composable
private fun SeasonMenuRow(season: EmbyShows.Season, selected: Boolean, onPick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    Row(
        Modifier.fillMaxWidth().height(42.dp)
            .padding(horizontal = 6.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(if (pressed) Color.Black.copy(alpha = .06f) else Color.Transparent)
            .clickable(interaction, indication = null) { onPick() }
            .padding(start = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Canvas(Modifier.size(20.dp)) {
            if (selected) {
                val s = size.width / 24f
                val w = 2.2f * s
                val ink = DesignTokens.textMain
                drawLine(ink, Offset(5f * s, 12.5f * s), Offset(10f * s, 17.5f * s), w, StrokeCap.Round)
                drawLine(ink, Offset(10f * s, 17.5f * s), Offset(19f * s, 7.5f * s), w, StrokeCap.Round)
            }
        }
        Spacer(Modifier.width(9.dp))
        Text(season.name, fontSize = 17.sp, color = DesignTokens.textMain,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
    }
}