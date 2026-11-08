package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import com.kivo.cinema.library.LocalVideoSource
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.PosterPalette
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.home.rememberHeroPullZoom
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** Detail data and foreground orchestration; media and curtain belong to DetailBackdrop. */
class MovieDetailActivity : ComponentActivity() {

    companion object {
        const val EXTRA_ID = "kivo.tmdbId"
        const val EXTRA_TV = "kivo.tmdbTv"

        fun intent(context: Context, item: com.kivo.cinema.tmdb.TmdbHome.Item): Intent =
            Intent(context, MovieDetailActivity::class.java)
                .putExtra(EXTRA_ID, item.id)
                .putExtra(EXTRA_TV, item.isTv)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setDarkStatusIcons(false)
        val id = intent.getIntExtra(EXTRA_ID, 0)
        val isTv = intent.getBooleanExtra(EXTRA_TV, false)
        val detail = TmdbDetail(TmdbClient(this))
        setContent {
            DetailPage(
                id = id, isTv = isTv,
                load = { detail.details(isTv, id) },
                onBack = { finish() },
                onDarkStatusIcons = { setDarkStatusIcons(it) },
            )
        }
    }

    private fun setDarkStatusIcons(dark: Boolean) {
        WindowCompat.getInsetsController(window, window.decorView)
            .isAppearanceLightStatusBars = dark
    }

    /** Fixed media, one continuous moving curtain, and an independent foreground list. */
    @Composable
    private fun DetailPage(id: Int, isTv: Boolean, load: () -> TmdbDetail.Details?,
                           onBack: () -> Unit, onDarkStatusIcons: (Boolean) -> Unit) {
        val context = LocalContext.current
        var details by remember { mutableStateOf<TmdbDetail.Details?>(null) }
        var error by remember { mutableStateOf<String?>(null) }
        var media by remember { mutableStateOf<MediaMatch.Matched?>(null) }
        var fieldColor by remember { mutableStateOf(DesignTokens.pageBackground) }
        LaunchedEffect(id, isTv) {
            val d = withContext(Dispatchers.IO) { load() }
            if (d == null) {
                error = "加载失败，请检查网络后重试"
            } else {
                details = d
                // Sample the same artwork used by the backdrop to avoid a color seam.
                val artUrl = d.heroArtworkUrl
                if (artUrl.isNotEmpty()) {
                    val bmp = withContext(Dispatchers.IO) { TmdbImageCache.load(artUrl) }
                    bmp?.let { fieldColor = PosterPalette.ambientOf(it) }
                }
                media = withContext(Dispatchers.IO) {
                    MediaMatch.find(LocalVideoSource.query(this@MovieDetailActivity), d.title)
                }
            }
        }
        val listState = rememberLazyListState()
        val density = LocalDensity.current
        val pull = rememberHeroPullZoom(
            atTop = { listState.firstVisibleItemIndex == 0 && listState.firstVisibleItemScrollOffset == 0 },
            maxPullPx = with(density) { 180.dp.toPx() })
        val pullDp = pull.distance.value / density.density
        val scrollDp = {
            if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset / density.density else 700f
        }
        // Fit to the captured Rex scroll sequence: lighten from the first consumed pixel.
        val sheetColor: State<Color> = remember(fieldColor) {
            derivedStateOf {
                androidx.compose.ui.graphics.lerp(fieldColor, DesignTokens.pageBackground,
                    ((scrollDp() - DETAIL_SHEET_START_DP) / DETAIL_SHEET_RAMP_DP)
                        .coerceIn(0f, 1f))
            }
        }
        val chromeDark by remember(sheetColor) { derivedStateOf { sheetColor.value.luminance() < .40f } }
        LaunchedEffect(chromeDark) { onDarkStatusIcons(!chromeDark) }
        val fg = if (chromeDark) Color.White else DesignTokens.textMain
        val fgSub = if (chromeDark) Color.White.copy(alpha = .72f) else DesignTokens.textSub

        Box(Modifier.fillMaxSize()) {
            DetailBackdrop(details, error, fg, sheetColor, scrollDp, pullDp)
            LazyColumn(
                state = listState,
                modifier = Modifier.fillMaxSize().nestedScroll(pull.connection),
                contentPadding = PaddingValues(bottom = 60.dp),
            ) {
                val d = details
                item(key = "hero-content") {
                  Column(Modifier.heightIn(min = 700.dp)) {
                    Spacer(Modifier.height((DETAIL_TITLE_TOP_DP + pullDp).dp))
                    if (d != null) {
                    TitleLayer(d, fg)
                    MetaLine(d, fg)
                        ActionRow(context, media, fg) { m ->
                            startActivity(Intent(this@MovieDetailActivity, DetailActivity::class.java)
                                .putExtra("kivo.media", m.item.path)
                                .putExtra("kivo.durationMs", m.item.durationMs))
                        }
                    if (d.overview.isNotBlank()) Overview(d.overview, fg)
                    if (d.rating > 0f) {
                        Box(Modifier.fillMaxWidth().padding(top = 14.dp)) {
                            RatingBadges(listOf("TMDB" to d.rating),
                                modifier = Modifier.fillMaxWidth(), alignCenter = true,
                                valueColor = fg)
                        }
                    }
                    val mm = media
                    if (mm != null) {
                        run {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(56.dp))
                                PlaySourceHeader(mm.quality, fg)
                                Spacer(Modifier.height(12.dp))
                                MediaCard(mm)
                                ExpandMediaInfoRow(fgSub)
                            }
                        }
                    } else {
                        run {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(70.dp))
                                EmptyResourceBlock(fg)
                            }
                        }
                    }
                    }
                  }
                }
                if (d != null) {
                    d.studioLogo?.let { url ->
                        item(key = "studio") {
                            Column(Modifier.fillMaxWidth()) {
                                StudioLogo(url, fg)
                            }
                        }
                    }
                    if (d.cast.isNotEmpty()) item(key = "cast") {
                        Column(Modifier.fillMaxWidth()) {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("演职员", fg)
                            Spacer(Modifier.height(14.dp))
                            CastRow(d.cast, fg, fgSub)
                        }
                    }
                    if (d.certifications.isNotEmpty() || d.contentNotes.isNotEmpty()) {
                        item(key = "guide") {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                                SectionHeader("家长指南", fg)
                                Spacer(Modifier.height(20.dp))
                                ParentalGuide(d.certifications, d.contentNotes)
                            }
                        }
                    }
                    if (d.genres.isNotEmpty()) item(key = "genres") {
                        Column(Modifier.fillMaxWidth()) {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("分类", fg)
                            Spacer(Modifier.height(12.dp))
                            GenreChips(d.genres)
                        }
                    }
                    d.trailerKey?.let { key ->
                        item(key = "trailer") {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                                SectionHeader("预告片", fg)
                                Spacer(Modifier.height(12.dp))
                                TrailerCard(key, "预告片", onClick = {
                                    context.startActivity(Intent(Intent.ACTION_VIEW,
                                        Uri.parse("https://www.youtube.com/watch?v=$key")))
                                })
                            }
                        }
                    }
                    if (d.stills.isNotEmpty()) item(key = "stills") {
                        Column(Modifier.fillMaxWidth()) {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("剧照", fg)
                            Spacer(Modifier.height(12.dp))
                            StillsRow(d.stills)
                        }
                    }
                    if (d.similar.isNotEmpty()) item(key = "similar") {
                        Column(Modifier.fillMaxWidth()) {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("相似作品", fg)
                            Spacer(Modifier.height(10.dp))
                            SimilarList(d.similar.take(8), fg, fgSub, onOpen = {})
                        }
                    }
                } else if (error != null) {
                    item(key = "error") {
                        Text(error!!, fontSize = 14.sp, color = fgSub,
                            modifier = Modifier.fillMaxWidth()
                                .padding(start = DETAIL_PAD_DP.dp, top = 16.dp))
                    }
                }
            }
            BackCircle(onBack)
        }
    }

}

/** 上层底色开始转浅的滚动量。 */
private const val DETAIL_SHEET_START_DP = 0f
/** 转浅斜坡长度。 */
private const val DETAIL_SHEET_RAMP_DP = 390f

