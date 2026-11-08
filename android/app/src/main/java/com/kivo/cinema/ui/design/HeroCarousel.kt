package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * hero 轮播（239 §4-v13；IMG_0138/0139/0140 三滚动态 + IMG_0142 对照播放器逐帧实测）：
 *
 * 层序（585dp 块内，自下而上）：
 *   0..585   海报 2:3 铺满原色全画（= /images 第一张——gAgWJhUS 与 Rex 0138 同图实证），
 *            **不加遮罩不加羽化**；
 *   外层     环境色遮罩（**盖在海报上层**——用户裁定 + IMG_0142 两播放器同构）：
 *            α 0（fy≤0.40）→ 0.55（0.55）→ 0.78（0.72）→ 0.96（1.0），
 *            海报下部透过遮罩隐约可见（f13 腿部/f1 肩部实证），底边被实色区完全盖住，
 *            上滑永不出现「海报底边一条线」；颜色 = 海报底缘平均色（随滚动向页底过渡）
 *   标题      TMDB logo（zh→null→en——0138 zh 红标实证），中心 fy≈0.69（帧 346-459pt），
 *            限高 115 限宽 78%；海报缺图才回退 34pt 文本（t22 实证）
 *   472      meta「类型 类型 · 日期」17pt 居中（帧 glyph 476-491.7）
 *   494      简介 15pt×2 行 lineHeight14 居中（帧 glyph 498.3-523）
 *   522      徽章行（块高 ~10 + 值 13pt）
 *   571.5    圆点中心：Ø7.3 中段、两端 6.0/3.3 收、间距 17.7（t22 实测 10 颗）
 *
 * 前景明暗：hero 组（meta/简介/徽章/logo）按环境色亮度 <0.55 定黑白，**整页不变**
 * （0138/0139 meta 都白；灰白底海报黑字）；圆点归 chrome 组（随滚动翻转——0138 白/0139 黑）。
 * 环境色/淡出系数由页面层下发（State 读取只触发重绘）。
 * 轮播不自动滚（用户实拍证伪 CSV M04-S01）。
 */
@Composable
fun HeroCarousel(
    candidates: List<TmdbHome.Item>,
    onAmbient: (Boolean, Color) -> Unit,          // (hero组黑白, 海报底缘色=环境色)
    dotsColor: Color,
    scrimColor: State<Color>,                     // 环境色（随滚动向页底过渡后的当前值）
    scrimAlpha: State<Float>,                     // 全局淡出系数 fade(S)
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    onOpenItem: (TmdbHome.Item) -> Unit = {},
) {
    if (candidates.isEmpty()) return
    val pagerState = rememberPagerState(pageCount = { candidates.size })
    var fgLight by remember { mutableStateOf(true) }
    var artUrl by remember { mutableStateOf<String?>(null) }
    var logoUrl by remember { mutableStateOf<String?>(null) }
    var artMissing by remember { mutableStateOf(false) }

    // 页稳定后：票选海报 → 底缘色（环境色）→ 前景黑白；logo 查询
    LaunchedEffect(pagerState.currentPage, candidates.size) {
        val m = candidates.getOrNull(pagerState.currentPage) ?: return@LaunchedEffect
        artUrl = m.posterHeroUrl
        logoUrl = null
        val resolved = withContext(Dispatchers.IO) {
            val top = runCatching { topPoster(m) }.getOrNull()
            val logo = runCatching { logoFor(m) }.getOrNull()
            Triple(top, logo, top ?: m.posterHeroUrl)
        }
        logoUrl = resolved.second
        artUrl = resolved.third
        val url = resolved.third
        val bmp = withContext(Dispatchers.IO) {
            TmdbImageCache.get(url) ?: com.kivo.cinema.tmdb.TmdbClient.staticBitmap(url)
        }
        artMissing = bmp == null
        if (bmp != null) {
            val amb = PosterPalette.ambientOf(bmp)
            // hero 组前景黑白：文字坐在环境色上——按环境色亮度判定
            // （0138 暗底缘→白；灰白底缘海报→黑）
            fgLight = amb.luminance() < 0.55f
            onAmbient(fgLight, amb)
        }
    }

    val fg = if (fgLight) Color.White else DesignTokens.textMain

    HorizontalPager(state = pagerState, modifier = Modifier.fillMaxWidth()) { idx ->
        val m = candidates[idx]
        Box(
            Modifier
                .fillMaxWidth()
                .height(HERO_HEIGHT_DP.dp)
        ) {
            // 海报：原色全画（外层环境色遮罩负责底部的溶入与底边隐藏）
            TmdbImage(
                artUrl ?: m.posterHeroUrl, m.title, Modifier.fillMaxSize(),
                onLoaded = { artMissing = it })
            // 环境色遮罩（海报外层）：海报自身标题/logo 区（fy≤0.55）完全透明，
            // 渐变跨 fy 0.55→0.65（0138：亮路径可见到 fy≈0.55、车辆区 fy 0.62 起已盖住），
            // fy≥0.65 实色 0.95——海报下部内容（英文标题 fy 0.82-0.92）与底边全部盖住
            Box(
                Modifier
                    .fillMaxSize()
                    .drawBehind {
                        val c = scrimColor.value
                        drawRect(Brush.verticalGradient(
                            0f to c.copy(alpha = 0f),
                            0.55f to c.copy(alpha = 0f),
                            0.65f to c.copy(alpha = 0.95f),
                            1f to c.copy(alpha = 0.97f)))
                    }
            )
            // 标题：logo 叠加（帧 346-459pt，中心 ~0.69×585）；海报缺图才回退文本
            val logo = logoUrl
            if (logo != null) {
                Box(
                    Modifier
                        .fillMaxWidth()
                        .offset(y = (LOGO_CENTER_DP - LOGO_MAX_H / 2f).dp),
                    contentAlignment = Alignment.TopCenter,
                ) {
                    TmdbImage(
                        logo, m.title,
                        Modifier
                            .padding(horizontal = 43.dp)
                            .height(LOGO_MAX_H.dp)
                            .fillMaxWidth(),
                        bg = Color.Transparent,
                        fit = true,
                    )
                }
            } else if (artMissing) {
                Box(
                    Modifier
                        .fillMaxWidth()
                        .offset(y = (TITLE_CENTER_DP - 30f).dp),
                    contentAlignment = Alignment.TopCenter,
                ) {
                    Text(m.title, fontSize = 34.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 1, overflow = TextOverflow.Ellipsis,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp))
                }
            }
            // meta / 简介（固定屏位）
            Column(
                Modifier
                    .fillMaxSize()
                    .padding(horizontal = 16.dp)
            ) {
                Spacer(Modifier.height(META_TOP_DP.dp))
                Text(m.metaLine, fontSize = 17.sp, lineHeight = 17.sp, color = fg,
                    textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
                if (m.overview.isNotBlank()) {
                    Text(m.overview, fontSize = 15.sp, lineHeight = 14.sp,
                        color = fg.copy(alpha = 0.92f), maxLines = 2,
                        overflow = TextOverflow.Ellipsis, textAlign = TextAlign.Center,
                        modifier = Modifier.fillMaxWidth())
                }
            }
            // 徽章行（固定 y522）
            Box(Modifier.offset(y = BADGE_TOP_DP.dp).fillMaxWidth()) {
                RatingBadges(listOf("TMDB" to m.rating), modifier = Modifier.fillMaxWidth(),
                    alignCenter = true, valueColor = fg)
            }
            // 圆点（中心 y574.8；颜色随 chrome 组在页面层翻转——0138 白/0139 黑）
            Box(
                Modifier
                    .offset(y = (DOTS_CENTER_DP - 3.65f).dp)
                    .fillMaxWidth(),
                contentAlignment = Alignment.Center,
            ) {
                androidx.compose.foundation.layout.Row(
                    horizontalArrangement = androidx.compose.foundation.layout.Arrangement.Center,
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    candidates.forEachIndexed { i, _ ->
                        val d = dotDiameter(i, candidates.size, pagerState.currentPage)
                        Box(
                            Modifier
                                .padding(horizontal = ((DOT_SPACING_DP - d) / 2f).dp)
                                .width(d.dp)
                                .height(d.dp)
                                .clip(CircleShape)
                                .background(dotsColor)
                        )
                    }
                }
            }
        }
    }
}

/* ---- 585dp 块内的固定屏位与遮罩参数（帧实测，239 §4-v13） ---- */
const val HERO_HEIGHT_DP = 585
const val LOGO_CENTER_DP = 402f          // 帧 346-459pt 中心 402.6
const val LOGO_MAX_H = 115f              // 两行 zh logo 帧高 112
const val TITLE_CENTER_DP = 398f
const val META_TOP_DP = 472f
const val BADGE_TOP_DP = 522f
const val DOTS_CENTER_DP = 574.5f
const val DOT_SPACING_DP = 17.7f

/** 圆点直径：中段 7.3、次端 6.0、端点 3.3（t22 实测）。 */
internal fun dotDiameter(index: Int, count: Int, active: Int): Float {
    val fromEnd = minOf(index, count - 1 - index)
    return when {
        fromEnd == 0 -> 3.3f
        fromEnd == 1 -> 6.0f
        else -> 7.3f
    }
}
