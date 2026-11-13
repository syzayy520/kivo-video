package com.kivo.cinema.ui.detail

import android.content.Intent
import android.net.Uri
import android.widget.Toast
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import com.kivo.cinema.media.marks.FilmMark
import com.kivo.cinema.ui.design.AppearancePrefs
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.PosterPalette
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import com.kivo.cinema.ui.design.home.navigationSource
import com.kivo.cinema.ui.design.home.rememberHeroPullZoom
import com.kivo.cinema.ui.detail.actions.DetailActionRow
import com.kivo.cinema.ui.detail.actions.DetailActions
import com.kivo.cinema.ui.detail.loading.DetailLoadFeedback
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 详情编排（254 D14，从 MovieDetailActivity 抽出）：固定幕布 + 连续滚动色场 + pull-zoom 头 + 节序。
 * **一套编排服务两种来源**：本地 TMDB 与 Emby 各产出 [DetailContent] 喂同一个页面；
 * 差异（资源槽/播放 intent/收藏来源/季集）都在 content 与 actions 里解析，页面本身无来源分支。
 */
@Composable
internal fun DetailPage(
    actions: DetailActions,
    load: () -> DetailContent?,
    onBack: () -> Unit,
    onDarkStatusIcons: (Boolean) -> Unit,
    openPage: (Intent) -> Unit,
) {
    val context = LocalContext.current
    var content by remember { mutableStateOf<DetailContent?>(null) }
    var error by remember { mutableStateOf<String?>(null) }
    var fieldColor by remember { mutableStateOf(DesignTokens.pageBackground) }
    var reload by remember { mutableStateOf(0) }
    var loading by remember { mutableStateOf(true) }
    LaunchedEffect(reload) {
        loading = true
        error = null
        try {
            val c = withContext(Dispatchers.IO) { load() }
            if (c == null) {
                error = "加载失败，请检查网络后重试"
            } else {
                content = c
                // Sample the same artwork used by the backdrop to avoid a color seam.
                val artUrl = c.details.heroArtworkUrl
                if (artUrl.isNotEmpty()) {
                    val bmp = withContext(Dispatchers.IO) {
                        c.loadImage?.invoke(artUrl) ?: TmdbImageCache.load(artUrl)
                    }
                    bmp?.let { fieldColor = PosterPalette.ambientOf(it) }
                }
            }
        } catch (e: CancellationException) { throw e }
        catch (_: Exception) { error = "无法获取影片详情，请检查网络后重试" }
        finally { loading = false }
    }
    val listState = rememberLazyListState()
    val density = LocalDensity.current
    val statusBarHeight = with(density) { WindowInsets.statusBars.getTop(this).toDp() }
    val pull = rememberHeroPullZoom(
        atTop = { listState.firstVisibleItemIndex == 0 && listState.firstVisibleItemScrollOffset == 0 },
        maxPullPx = with(density) { 180.dp.toPx() })
    val pullDp = pull.distance.value / density.density
    val scrollDp = {
        if (listState.firstVisibleItemIndex == 0)
            listState.firstVisibleItemScrollOffset / density.density else 700f
    }
    // Fit to the captured scroll sequence: lighten from the first consumed pixel.
    val sheetColor: State<Color> = remember(fieldColor) {
        derivedStateOf {
            lerp(fieldColor, DesignTokens.pageBackground,
                ((scrollDp() - DETAIL_SHEET_START_DP) / DETAIL_SHEET_RAMP_DP).coerceIn(0f, 1f))
        }
    }
    val chromeDark by remember(sheetColor) { derivedStateOf { sheetColor.value.luminance() < .40f } }
    LaunchedEffect(chromeDark) { onDarkStatusIcons(!chromeDark) }
    val fg = if (chromeDark) Color.White else DesignTokens.textMain
    val fgSub = if (chromeDark) Color.White.copy(alpha = .72f) else DesignTokens.textSub
    val d = content?.details
    val media = content?.media
    val quality = content?.quality ?: ""
    val playIntent = content?.playIntent
    val tmdbIsTv = content?.tmdbIsTv ?: false
    val noPlayReason = content?.noPlayReason ?: DETAIL_NO_LOCAL_PLAY
    val seasons = content?.seasons ?: emptyList()
    val loadEpisodes = content?.loadEpisodes
    val ss = rememberSeasonState(content)
    var menuOpen by remember { mutableStateOf(false) }
    var menuAnchor by remember { mutableStateOf(Rect.Zero) }
    val overlay = remember { DetailOverlayState() }
    val glassSources = remember { NavigationBackdropState() }
    val today = remember { java.time.LocalDate.now() }
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    val curSeason = ss.season
    val nextEp = ss.episodes.firstOrNull { !it.played && it.positionTicks > 0 }
        ?: ss.episodes.firstOrNull { !it.played } ?: ss.episodes.firstOrNull()
    val playSubtitle = if (loadEpisodes != null && curSeason != null && nextEp != null)
        "第${curSeason.index}季·${nextEp.index}集" else null
    val calendarLabel = remember(ss.episodes, today) {
        DetailCalendarData.barLabel(ss.episodes, today)
    }

    Box(Modifier.fillMaxSize()) {
        Box(Modifier.navigationSource(glassSources)) {
            DetailBackdrop(d, sheetColor, scrollDp, pullDp, content?.loadImage)
        }
        LazyColumn(
            state = listState,
            modifier = Modifier.padding(top = statusBarHeight)
                .fillMaxSize().nestedScroll(pull.connection)
                .navigationSource(glassSources),
            contentPadding = PaddingValues(bottom = 60.dp),
        ) {
            item(key = "hero-content") {
                Column(Modifier.heightIn(min = 700.dp - statusBarHeight)) {
                    // Keep the accepted hero coordinates while clipping scroll content below system icons.
                    Spacer(Modifier.height((DETAIL_TITLE_TOP_DP + pullDp).dp - statusBarHeight))
                    if (d != null) {
                        TitleLayer(d, fg)
                        MetaLine(d, fg)
                        DetailActionRow(playIntent != null, fg, actions,
                            playSubtitle = playSubtitle,
                            onPlay = {
                                if (playIntent == null) Toast.makeText(context,
                                    noPlayReason, Toast.LENGTH_LONG).show()
                                else openPage(playIntent)
                            },
                            onOpenMore = { overlay.openMore(it) })
                        if (d.overview.isNotBlank()) Overview(d.overview, fg)
                        if (d.rating > 0f) {
                            Box(Modifier.fillMaxWidth().padding(top = 14.dp)) {
                                RatingBadges(listOf(d.ratingSource to d.rating),
                                    modifier = Modifier.fillMaxWidth(), alignCenter = true, valueColor = fg)
                            }
                        }
                        if (media != null) {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(56.dp))
                                PlaySourceHeader(quality, fg)
                                Spacer(Modifier.height(12.dp))
                                MediaCard(media)
                                ExpandMediaInfoRow(fgSub, media.streams)
                            }
                        } else if (loadEpisodes != null) {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(40.dp))
                                SeasonEpisodeSection(seasons, ss.season, ss.episodes,
                                    ss.loading, ss.error, fg, fgSub, content?.loadImage,
                                    calendarLabel = calendarLabel,
                                    onOpenCalendar = { overlay.calendarOpen = true },
                                    menuOpen = menuOpen,
                                    onOpenMenu = { menuOpen = true },
                                    onAnchor = { menuAnchor = it },
                                    onPickSeason = { ss.season = it },
                                    onRetry = { ss.retry() },
                                    onPlay = { ep -> content?.playEpisode?.let { openPage(it(ep)) } })
                            }
                        } else {
                            Column(Modifier.fillMaxWidth()) {
                                Spacer(Modifier.height(70.dp))
                                EmptyResourceBlock(fg, actions.marks.watchlist, actions.enabled) {
                                    actions.toggle(FilmMark.WATCHLIST)
                                }
                            }
                        }
                    }
                }
            }
            if (d != null) {
                d.studioLogo?.let { url ->
                    item(key = "studio") { Column(Modifier.fillMaxWidth()) { StudioLogo(url, fg) } }
                }
                if (d.cast.isNotEmpty()) item(key = "cast") {
                    Column(Modifier.fillMaxWidth()) {
                        Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                        SectionHeader("演职员", fg)
                        Spacer(Modifier.height(14.dp))
                        CastRow(d.cast, fg, fgSub, content?.loadImage)
                    }
                }
                if (AppearancePrefs.showContentNotes &&
                    (d.certifications.isNotEmpty() || d.contentNotes.isNotEmpty())) {
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
                            SectionHeader("预告片 · YouTube", fg)
                            Spacer(Modifier.height(12.dp))
                            TrailerCard(key, "预告片", onClick = {
                                openPage(Intent(Intent.ACTION_VIEW,
                                    Uri.parse("https://www.youtube.com/watch?v=$key")))
                            })
                        }
                    }
                }
                if (AppearancePrefs.showStills && d.stills.isNotEmpty()) item(key = "stills") {
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
                        SimilarList(d.similar.filter { it.id > 0 && it.id != d.id }.take(8), fg, fgSub,
                            onOpen = { similar ->
                                openPage(Intent(context, MovieDetailActivity::class.java)
                                    .putExtra(MovieDetailActivity.EXTRA_ID, similar.id)
                                    .putExtra(MovieDetailActivity.EXTRA_TV, tmdbIsTv))
                            })
                    }
                }
            }
        }
        if (content == null && loading) CircularProgressIndicator(
            modifier = Modifier.align(Alignment.Center), color = fg)
        DetailLoadFeedback(error, content != null, loading,
            onRetry = { if (!loading) reload++ },
            modifier = if (content != null) Modifier.align(Alignment.BottomCenter)
                .navigationBarsPadding().padding(16.dp)
            else Modifier.align(Alignment.Center).padding(horizontal = 24.dp))
        SeasonMenuOverlay(menuOpen, menuAnchor, seasons, ss.season, glassSources,
            onDismiss = { menuOpen = false },
            onPick = { menuOpen = false; ss.season = it })
        DetailOverlayHost(overlay, content, ss.season, ss.episodes, actions, glassSources, openPage)
        BackCircle(onBack)
    }
}

/** 上层底色开始转浅的滚动量。 */
private const val DETAIL_SHEET_START_DP = 0f
/** 转浅斜坡长度。 */
private const val DETAIL_SHEET_RAMP_DP = 390f