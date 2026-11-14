package com.kivo.cinema.ui.detail

import android.content.Intent
import android.widget.Toast
import androidx.activity.compose.BackHandler
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
    val today = remember { java.time.LocalDate.now() }
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    val curSeason = ss.season
    val nextEp = ss.episodes.firstOrNull { !it.played && it.positionTicks > 0 }
        ?: ss.episodes.firstOrNull { !it.played } ?: ss.episodes.firstOrNull()
    val playSubtitle = if (loadEpisodes != null && curSeason != null && nextEp != null)
        "第${curSeason.index}季·${nextEp.index}集" else null
    // 播放钮与副标题必须指向**同一集**（272 审计 D1）：
    // 副标题报的是"下一集"，播放钮却总开第一集 ⇒ 按钮说的和做的不是一回事。
    val ctaPlayable = if (loadEpisodes != null) nextEp != null else playIntent != null
    val calendarLabel = remember(ss.episodes, today) {
        DetailCalendarData.barLabel(ss.episodes, today)
    }

    Box(Modifier.fillMaxSize()) {
        Box {
            DetailBackdrop(d, sheetColor, scrollDp, pullDp, content?.loadImage)
        }
        LazyColumn(
            state = listState,
            modifier = Modifier.padding(top = statusBarHeight)
                .fillMaxSize().nestedScroll(pull.connection),
            contentPadding = PaddingValues(bottom = 60.dp),
        ) {
            item(key = "hero-content") {
                Column(Modifier.heightIn(min = 700.dp - statusBarHeight)) {
                    // Keep the accepted hero coordinates while clipping scroll content below system icons.
                    Spacer(Modifier.height((DETAIL_TITLE_TOP_DP + pullDp).dp - statusBarHeight))
                    if (d != null) {
                        TitleLayer(d, fg)
                        MetaLine(d, fg)
                        DetailActionRow(ctaPlayable, fg, actions,
                            playSubtitle = playSubtitle,
                            onPlay = {
                                val target = if (loadEpisodes != null) {
                                    nextEp?.let { ep -> content?.playEpisode?.invoke(ep) }
                                } else playIntent
                                if (target == null) Toast.makeText(context,
                                    noPlayReason, Toast.LENGTH_LONG).show()
                                else openPage(target)
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
                detailLowerSections(d, fg, fgSub, tmdbIsTv, content?.loadImage, openPage)
            }
        }
        if (content == null && loading) CircularProgressIndicator(
            modifier = Modifier.align(Alignment.Center), color = fg)
        DetailLoadFeedback(error, content != null, loading,
            onRetry = { if (!loading) reload++ },
            modifier = if (content != null) Modifier.align(Alignment.BottomCenter)
                .navigationBarsPadding().padding(16.dp)
            else Modifier.align(Alignment.Center).padding(horizontal = 24.dp))
        SeasonMenuOverlay(menuOpen, menuAnchor, seasons, ss.season,
            onDismiss = { menuOpen = false },
            onPick = { menuOpen = false; ss.season = it })
        DetailOverlayHost(overlay, content, ss.season, ss.episodes, actions, openPage)
        // 返回键先关浮层（272 审计 D3）：日历 → 更多菜单 → 选季菜单 → 才退出详情页。
        BackHandler(enabled = overlay.calendarOpen || overlay.moreOpen || menuOpen) {
            when {
                overlay.calendarOpen -> overlay.calendarOpen = false
                overlay.moreOpen -> overlay.moreOpen = false
                else -> menuOpen = false
            }
        }
        BackCircle(onBack)
    }
}

/** 上层底色开始转浅的滚动量。 */
private const val DETAIL_SHEET_START_DP = 0f
/** 转浅斜坡长度。 */
private const val DETAIL_SHEET_RAMP_DP = 390f