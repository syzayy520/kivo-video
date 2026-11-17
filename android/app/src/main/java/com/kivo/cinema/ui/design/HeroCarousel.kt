package com.kivo.cinema.ui.design

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.PagerDefaults
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.home.HeroPage
import com.kivo.cinema.ui.motion.MotionGate
import kotlin.math.abs
import kotlin.math.floor

/**
 * Paging, active artwork reporting, and the shared page indicator.
 *
 * @param onOpen hero 整卡点击。null = 不可点（Emby 之外的旧调用保持原行为）。
 *   点击判定放在 [HeroPage] 根 Box 内部而不是这里包一层 —— 见 HeroPage 的
 *   「封面加载失败→重试」优先级说明。
 */
@Composable
fun HeroCarousel(
    cards: List<com.kivo.cinema.ui.design.home.HeroCard>,
    onAmbient: (Color) -> Unit,
    dotsColor: Color,
    scrimColor: State<Color>,
    pullDp: Float = 0f,
    scrollPx: () -> Float = { 0f },
    previewVisible: Boolean = true,
    previewEnabled: Boolean = false,
    generation: Int = 0,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
    /**
     * 317 · 把中立 [HeroCard] 还原成 TMDB 条目（只用于预告片）。
     * 返回 null = 该来源没有预告能力（Emby 服务器首页）。
     */
    trailerItemFor: (com.kivo.cinema.ui.design.home.HeroCard) -> TmdbHome.Item? = { null },
    /** 317 · 异步取片名艺术字；null = 用 card.logoUrl（Emby 侧）。 */
    logoLoader: (suspend (com.kivo.cinema.ui.design.home.HeroCard) -> String?)? = null,
    /**
     * 点击海报打开详情。null = 不可点（保持旧行为）。
     *
     * 【为什么现在才有】改造前 hero 只是展示墙，点它没反应；
     * Emby 服务器首页需要"点海报进详情"，所以把它做成可选回调而不是写死。
     */
    onOpen: ((com.kivo.cinema.ui.design.home.HeroCard) -> Unit)? = null,
) {
    if (cards.isEmpty()) return
    // 分页点补间也属 G1 门控范围：App 开关或系统关动画时取 0（瞬时切换）。
    val context = LocalContext.current
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val dotMorphMs = if (reduceMotion) 0 else DOT_MORPH_MS
    val pagerState = rememberPagerState(pageCount = { cards.size })
    val mediaKeys = cards.map { it.id }
    val palettes = remember(generation, mediaKeys) { mutableStateMapOf<Int, Color>() }
    val report by rememberUpdatedState(onAmbient)
    LaunchedEffect(pagerState, generation, mediaKeys) {
        snapshotFlow {
            val position = (pagerState.currentPage + pagerState.currentPageOffsetFraction)
                .coerceIn(0f, (cards.size - 1).toFloat())
            val left = floor(position).toInt()
            val right = (left + 1).coerceAtMost(cards.lastIndex)
            val a = palettes[left] ?: palettes[right]
            val b = palettes[right] ?: palettes[left]
            if (a != null && b != null) lerp(a, b, position - left) else null
        }.collect { color -> color?.let { report(it) } }
    }
    Box(Modifier.fillMaxWidth().height((HERO_HEIGHT_DP + HERO_TAIL_GAP_DP + pullDp).dp)) {
        HorizontalPager(state = pagerState, key = { "$generation:${cards[it].id}" },
            beyondViewportPageCount = 1,
            flingBehavior = PagerDefaults.flingBehavior(pagerState,
                snapAnimationSpec = spring(dampingRatio = 1f, stiffness = 240f)),
            modifier = Modifier.fillMaxSize()) { index ->
            HeroPage(
                card = cards[index],
                active = index == pagerState.settledPage && !pagerState.isScrollInProgress && previewVisible,
                scrimColor = scrimColor,
                onAmbient = { palettes[index] = it },
                pullDp = pullDp,
                proxyAddress = proxyAddress,
                previewEnabled = previewEnabled,
                scrollPx = scrollPx,
                pageOffset = { pagerState.currentPage - index + pagerState.currentPageOffsetFraction },
                trailerItem = trailerItemFor(cards[index]),
                trailerFor = trailerFor,
                logoLoader = logoLoader?.let { loader -> suspend { loader(cards[index]) } },
                onOpen = onOpen,
            )
        }
        Row(Modifier.align(Alignment.TopCenter).offset(y = (DOTS_CENTER_DP - 3.65f + pullDp).dp),
            verticalAlignment = Alignment.CenterVertically) {
            cards.forEachIndexed { index, _ ->
                val position = pagerState.currentPage + pagerState.currentPageOffsetFraction
                val weight = (1f - abs(index - position)).coerceIn(0f, 1f)
                val left = floor(position).toInt().coerceIn(0, cards.lastIndex)
                val right = (left + 1).coerceAtMost(cards.lastIndex)
                val fraction = (position - left).coerceIn(0f, 1f)
                val target = dotDiameter(index, cards.size, left) * (1f - fraction) +
                    dotDiameter(index, cards.size, right) * fraction
                // DISC-02 ✅：选中点 9dp、未选中 7.3dp（尾部仍按 3.3/6 收缩），
                // 尺寸变化走 150ms 补间 —— 只靠 alpha 区分看不出当前页。
                val diameter by animateDpAsState(target.dp, tween(dotMorphMs), label = "dot")
                Box(Modifier.width(DOT_SPACING_DP.dp).height(DOT_HOLDER_DP.dp),
                    contentAlignment = Alignment.Center) {
                    Box(Modifier.size(diameter).clip(CircleShape)
                        .background(dotsColor.copy(alpha = .35f + .65f * weight)))
                }
            }
        }
    }
}

const val HERO_HEIGHT_DP = 585
/**
 * 文字标题顶边（V2）：原艺术字分支居中于 402dp，文字标题按 34sp 字高取 368dp。
 * 与 BADGE_TOP_DP 的 154dp 落差保留元信息/简介两行的位置不动。
 */
const val TITLE_TOP_DP = 368f
const val META_TOP_DP = 472f
const val BADGE_TOP_DP = 522f
const val DOTS_CENTER_DP = 574.5f
const val DOT_SPACING_DP = 17.7f

/**
 * hero 卡底到下一个 section 标题的**总**间距 20dp（V2 口径）。
 *
 * 下一节（[com.kivo.cinema.ui.design.home.sections.PosterSection] /
 * [com.kivo.cinema.ui.design.home.sections.ContinueRow]）自带 13dp 顶距且不归本卡管，
 * 所以这里只补差额 7dp —— 13 + 7 = 20。左右 13dp 与卡内布局一律不动。
 */
internal const val HERO_TAIL_GAP_DP = 7f

/** 分页点直径（dp）：选中 9，未选中 7.3，尾部两枚按 3.3/6 收缩（E11 帧序列）。 */
internal const val DOT_ACTIVE_DP = 9f
internal const val DOT_IDLE_DP = 7.3f
private const val DOT_NEAR_EDGE_DP = 6f
private const val DOT_FAR_EDGE_DP = 3.3f

/** 点的容器高度要容得下选中直径，否则 9dp 会溢出 Row 的可测高度。 */
private const val DOT_HOLDER_DP = 9f
private const val DOT_MORPH_MS = 150

internal fun dotDiameter(index: Int, count: Int, active: Int): Float {
    if (index == active) return DOT_ACTIVE_DP
    // Shrink the remote end; near the final page the leading end recedes instead.
    val edgeDistance = if (active >= count - 3) index else count - 1 - index
    return when (edgeDistance) {
        0 -> DOT_FAR_EDGE_DP
        1 -> DOT_NEAR_EDGE_DP
        else -> DOT_IDLE_DP
    }
}