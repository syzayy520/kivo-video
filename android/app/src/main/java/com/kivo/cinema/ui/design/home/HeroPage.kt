package com.kivo.cinema.ui.design.home


import android.graphics.Bitmap
import androidx.compose.foundation.clickable
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.*
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.*
import com.kivo.cinema.ui.design.home.trailer.HeroTrailer
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.math.abs

/** 片名艺术字（TMDB logo）：居中于 402dp、最大高 115dp、左右 43dp（首页原口径）。 */
private val LOGO_CENTER_DP = 402f
private val LOGO_MAX_H = 115f

/** A pager page owns its resources, including while a neighboring page is composed. */
@Composable
internal fun HeroPage(
    card: HeroCard,
    active: Boolean,
    scrimColor: State<Color>,
    onAmbient: (Color) -> Unit,
    pullDp: Float,
    proxyAddress: String?,
    previewEnabled: Boolean,
    scrollPx: () -> Float,
    pageOffset: () -> Float,
    /**
     * 317 · TMDB 专有的预告片数据。null = 该来源没有预告能力（Emby 服务器首页）。
     *
     * 【为什么单独一个参数而不是塞进 [HeroCard]】预告片依赖一整套 TMDB 运行时
     * （`HeroTrailer` / `TrailerResult` / proxy 地址），把它抽象进中立模型会把
     * Emby 也拖进 TMDB 的依赖里 —— 那就不是"抽象"而是"绑死"。
     */
    trailerItem: TmdbHome.Item? = null,
    trailerFor: (TmdbHome.Item) -> TrailerResult = { TrailerResult() },
    /**
     * 317 · 异步取片名艺术字。null = 直接用 [HeroCard.logoUrl]。
     *
     * 【为什么还要这个】TMDB 的 logo 是**按 id 异步查**的（不在列表接口里），
     * 改成同步字段会让首页丢掉艺术字标题（回归）。Emby 侧图 URL 直接给得出 ⇒ 传 null。
     */
    logoLoader: (suspend () -> String?)? = null,
    /** 整卡点击（DISC-08）：封面已加载时才挂，避免抢走「点击重试」的手势。 */
    onOpen: ((HeroCard) -> Unit)? = null,
) {
    var artwork by remember(card.id) { mutableStateOf<Bitmap?>(null) }
    var logo by remember(card.id) { mutableStateOf(card.logoUrl) }
    var finished by remember(card.id) { mutableStateOf(false) }
    var ambient by remember(card.id) { mutableStateOf<Color?>(null) }
    var imageAttempt by remember(card.id) { mutableIntStateOf(0) }
    val report by rememberUpdatedState(onAmbient)
    LaunchedEffect(card.id, imageAttempt) {
        finished = false
        val resolved = withContext(Dispatchers.IO) {
            if (card.heroUrl.isBlank()) null else TmdbImageCache.load(card.heroUrl)
        }
        artwork = resolved
        ambient = resolved?.let { withContext(Dispatchers.Default) { PosterPalette.ambientOf(it) } }
        finished = true
    }
    LaunchedEffect(card.id, imageAttempt) {
        val loader = logoLoader
        logo = if (loader != null) withContext(Dispatchers.IO) { loader() } else card.logoUrl
    }
    LaunchedEffect(ambient) {
        ambient?.let { report(it) }
    }
    val fg = if (HomeAtmosphere.lightForeground(scrimColor.value)) Color.White else DesignTokens.textMain
    Box(Modifier.fillMaxWidth().height((HERO_HEIGHT_DP + pullDp).dp).clipToBounds()) {
        Box(Modifier.fillMaxWidth().height(HERO_HEIGHT_DP.dp).graphicsLayer {
            // The foreground scrolls normally; only the media cancels list motion.
            translationY = scrollPx() * .95f
            val stretch = 1f + pullDp / HERO_HEIGHT_DP
            scaleX = stretch; scaleY = stretch
            transformOrigin = TransformOrigin(.5f, 0f)
        }) {
            artwork?.let { bitmap ->
                Image(bitmap.asImageBitmap(), card.title,
                    contentScale = ContentScale.Crop,
                    modifier = Modifier.fillMaxSize().graphicsLayer {
                        val offset = pageOffset().coerceIn(-1f, 1f)
                        val zoom = 1.144f * (1f + .025f * abs(offset))
                        scaleX = zoom; scaleY = zoom
                        translationX = size.width * .055f * offset
                    })
            }
            val ti = trailerItem
            if (previewEnabled && ti != null) {
                HeroTrailer(ti, active, artwork != null, trailerFor, proxyAddress,
                    Modifier.fillMaxSize(), onPlayingChanged = {})
            }
        }
        Box(Modifier.fillMaxSize().drawBehind {
            val c = scrimColor.value
            drawRect(Brush.verticalGradient(
                0f to c.copy(alpha = 0f),
                .47f to c.copy(alpha = 0f),
                .60f to c.copy(alpha = .28f),
                .75f to c.copy(alpha = .80f),
                .80f to c.copy(alpha = .98f),
                .84f to c,
                .92f to c,
                1f to c))
        })
        Box(Modifier.fillMaxSize().graphicsLayer {
            val offset = pageOffset().coerceIn(-1f, 1f)
            translationX = -size.width * .06f * offset
            // 相邻页必须完全隐藏：旧公式 `1 − .65·|offset|` 静态时仍有 35% 不透明度，
            // 两页各带一个片名 ⇒ 屏幕上「一上一下两个片名」（2026-11-17 真机确认）。
            alpha = (1f - 2f * abs(offset)).coerceIn(0f, 1f)
        }.then(
            if (onOpen != null && artwork != null) Modifier.clickable { onOpen(card) } else Modifier
        )) {
            if (!finished && artwork == null) Text("封面加载中…", color = fg, fontSize = 12.sp,
                modifier = Modifier.align(Alignment.TopCenter).padding(top = 90.dp))
            if (finished && artwork == null) Text("封面加载失败，点击重试", color = fg, fontSize = 12.sp,
                modifier = Modifier.align(Alignment.TopCenter).padding(top = 90.dp)
                    .clickable { imageAttempt++ }.padding(12.dp))
            if (logo != null) {
                Box(Modifier.fillMaxWidth().offset(y = (LOGO_CENTER_DP - LOGO_MAX_H / 2 + pullDp).dp),
                    contentAlignment = Alignment.TopCenter) {
                    TmdbImage(logo!!, card.title,
                        Modifier.padding(horizontal = 43.dp).height(LOGO_MAX_H.dp).fillMaxWidth(),
                        bg = Color.Transparent, fit = true, onLoaded = { if (!it) logo = null })
                }
            } else {
                Text(card.title, color = fg, fontSize = 34.sp, fontWeight = FontWeight.Bold,
                    textAlign = TextAlign.Center, maxLines = 2,
                    // P0-3：Emby 服务器名可能是「中文\n无空格长英文注音」，缺 overflow 会左右溢出屏幕。
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.fillMaxWidth().offset(y = (368 + pullDp).dp).padding(horizontal = 16.dp))
            }
            Column(Modifier.fillMaxWidth().offset(y = (META_TOP_DP + pullDp).dp).padding(horizontal = 16.dp)) {
                Text(card.metaLine, fontSize = 14.sp, lineHeight = 20.sp, color = fg,
                    textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
                // REX 0454：简介为纯 overview 文本，不带「片名」前缀（1:1 对齐）。
                if (card.overview.isNotBlank()) Text(card.overview, fontSize = 12.sp, lineHeight = 14.sp,
                    color = fg.copy(alpha = .92f), maxLines = 2, overflow = TextOverflow.Ellipsis,
                    textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
            }
            // P0-3：无评分时隐藏整块徽章，不再显示假的「Emby 0.0」。
            card.rating?.takeIf { it > 0f }?.let { score ->
                Box(Modifier.offset(y = (BADGE_TOP_DP + pullDp).dp).fillMaxWidth()) {
                    RatingBadges(
                        listOf(card.ratingSource to score.toFloat()), Modifier.fillMaxWidth(),
                        alignCenter = true, valueColor = fg)
                }
            }
        }
    }
}
