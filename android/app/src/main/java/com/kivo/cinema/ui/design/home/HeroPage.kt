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

/** A pager page owns its resources, including while a neighboring page is composed. */
@Composable
internal fun HeroPage(
    item: TmdbHome.Item,
    active: Boolean,
    scrimColor: State<Color>,
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    onAmbient: (Color) -> Unit,
    pullDp: Float,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
    previewEnabled: Boolean,
    scrollPx: () -> Float,
) {
    var artwork by remember(item.isTv, item.id) { mutableStateOf<Bitmap?>(null) }
    var logo by remember(item.isTv, item.id) { mutableStateOf<String?>(null) }
    var finished by remember(item.isTv, item.id) { mutableStateOf(false) }
    var ambient by remember(item.isTv, item.id) { mutableStateOf<Color?>(null) }
    var imageAttempt by remember(item.isTv, item.id) { mutableIntStateOf(0) }
    val report by rememberUpdatedState(onAmbient)
    LaunchedEffect(item.isTv, item.id, imageAttempt) {
        finished = false
        val resolved = withContext(Dispatchers.IO) {
            val art = topPoster(item) ?: item.posterHeroUrl
            val title = logoFor(item)
            TmdbImageCache.load(art) to title
        }
        artwork = resolved.first
        logo = resolved.second
        ambient = resolved.first?.let { withContext(Dispatchers.Default) { PosterPalette.ambientOf(it) } }
        finished = true
    }
    LaunchedEffect(active, ambient) {
        if (active) ambient?.let { report(it) }
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
                Image(bitmap.asImageBitmap(), item.title,
                    contentScale = ContentScale.Crop,
                    modifier = Modifier.fillMaxSize().graphicsLayer { scaleX = 1.144f; scaleY = 1.144f })
            }
            if (previewEnabled) HeroTrailer(item, active, artwork != null, trailerFor, proxyAddress,
                Modifier.fillMaxSize(), onPlayingChanged = {})
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
        if (finished && artwork == null) Text("封面加载失败，点击重试", color = fg, fontSize = 12.sp,
            modifier = Modifier.align(Alignment.TopCenter).padding(top = 90.dp)
                .clickable { imageAttempt++ }.padding(12.dp))
        if (logo != null) {
            Box(Modifier.fillMaxWidth().offset(y = (LOGO_CENTER_DP - LOGO_MAX_H / 2 + pullDp).dp),
                contentAlignment = Alignment.TopCenter) {
                TmdbImage(logo!!, item.title,
                    Modifier.padding(horizontal = 43.dp).height(LOGO_MAX_H.dp).fillMaxWidth(),
                    bg = Color.Transparent, fit = true, onLoaded = { if (!it) logo = null })
            }
        } else if (finished && logo == null) {
            Text(item.title, color = fg, fontSize = 34.sp, fontWeight = FontWeight.Bold,
                textAlign = TextAlign.Center, maxLines = 2,
                modifier = Modifier.fillMaxWidth().offset(y = (368 + pullDp).dp).padding(horizontal = 16.dp))
        }
        Column(Modifier.fillMaxWidth().offset(y = (META_TOP_DP + pullDp).dp).padding(horizontal = 16.dp)) {
            Text(item.metaLine, fontSize = 14.sp, lineHeight = 20.sp, color = fg,
                textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
            if (item.overview.isNotBlank()) Text("「${item.title}」${item.overview}", fontSize = 12.sp, lineHeight = 14.sp,
                color = fg.copy(alpha = .92f), maxLines = 2, overflow = TextOverflow.Ellipsis,
                textAlign = TextAlign.Center, modifier = Modifier.fillMaxWidth())
        }
        Box(Modifier.offset(y = (BADGE_TOP_DP + pullDp).dp).fillMaxWidth()) {
            RatingBadges(listOf("TMDB" to item.rating), Modifier.fillMaxWidth(),
                alignCenter = true, valueColor = fg)
        }
    }
}
