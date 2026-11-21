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
    previewContent: DetailContent? = null,
    onBack: () -> Unit,
    onDarkStatusIcons: (Boolean) -> Unit,
    openPage: (Intent) -> Unit,
) {
    val context = LocalContext.current
    // 336 R2：列表快照先上屏（首帧即有海报/标题/评分），完整详情后台补。
    var content by remember { mutableStateOf<DetailContent?>(previewContent) }
    var error by remember { mutableStateOf<String?>(null) }
    // 336 R2：有 preview 时首帧幕布已是深色海报，底色中性深灰起步（白字可读、无浅灰块），
    // 吸色 effect 随即改成真实海报色；无 preview 维持原浅色初始。
    var fieldColor by remember {
        mutableStateOf(if (previewContent != null) Color(0xFF1A1A1F) else DesignTokens.pageBackground)
    }
    // P0-1：海报文字带亮度采样。-1 = 无海报/未加载，退回原 sheet 判定。
    var posterBandLum by remember { mutableStateOf(-1f) }
    var reload by remember { mutableStateOf(0) }
    var loading by remember { mutableStateOf(previewContent == null) }
    LaunchedEffect(reload) {
        loading = true
        error = null
        try {
            com.kivo.cinema.net.NetDiag.log("detail-perf load-start reload=$reload preview=${previewContent != null}")
            val c = withContext(Dispatchers.IO) { load() }
            com.kivo.cinema.net.NetDiag.log("detail-perf load-done ok=${c != null}")
            if (c == null) {
                // 有 preview 也置 error：DetailLoadFeedback 据此给出 Snackbar 重试。
                // 文案不带"已保留当前内容"，由反馈组件统一追加，避免重复。
                error = if (content == null) "加载失败，请检查网络后重试" else "详情加载失败"
            } else {
                content = c
            }
        } catch (e: CancellationException) { throw e }
        catch (_: Exception) {
            error = if (content == null) "无法获取影片详情，请检查网络后重试" else "详情加载失败"
        } finally { loading = false }
    }
    // 336 R2：吸色跟随当前幕布 URL —— preview 上屏当帧就采（图已在磁盘缓存，毫秒级），
    // 避免"海报已显示但底色还是 pageBackground"的浅灰块；完整详情替换后 URL 变了再采一次。
    val artUrl = content?.details?.heroArtworkUrl.orEmpty()
    val artLoader = content?.loadImage
    LaunchedEffect(artUrl, artLoader) {
        if (artUrl.isBlank()) return@LaunchedEffect
        val bmp = withContext(Dispatchers.IO) { artLoader?.invoke(artUrl) ?: TmdbImageCache.load(artUrl) }
        if (bmp != null) {
            fieldColor = PosterPalette.ambientOf(bmp)
            posterBandLum = withContext(Dispatchers.IO) { HeroForeground.textBandLuminance(bmp) }
        }
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
    // 状态图标口径不变（仍看 sheetColor），避免回归；仅 hero 文字改用合成背景判定。
    val statusDark by remember(sheetColor) { derivedStateOf { sheetColor.value.luminance() < .40f } }
    LaunchedEffect(statusDark) { onDarkStatusIcons(!statusDark) }
    // P0-1：文字画在「海报 + field 渐变」合成层上，不能只看 sheetColor（海报环境色偏暗）。
    // 未滚动时用文字带采样亮度，随滚动按同一斜坡过渡到 sheet 亮度；无采样时退回原判定。
    val sheetRamp = ((scrollDp() - DETAIL_SHEET_START_DP) / DETAIL_SHEET_RAMP_DP).coerceIn(0f, 1f)
    val sheetLum = sheetColor.value.luminance()
    val effectiveLum = if (posterBandLum >= 0f) {
        val heroLum = HeroForeground.heroBandLuminance(posterBandLum, fieldColor)
        heroLum + (sheetLum - heroLum) * sheetRamp
    } else sheetLum
    val fg = if (HeroForeground.needsLight(effectiveLum)) Color.White else DesignTokens.textMain
    val fgSub = if (HeroForeground.needsLight(effectiveLum)) Color.White.copy(alpha = .72f) else DesignTokens.textSub
    val d = content?.details
    val isPreview = content?.preview == true
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
    // 317 · 观看进度：有位置就显示「继续观看 12:34」，否则仍是「播放」。
    // 位置来自服务器 UserData（跨客户端通用）或本地记录，见 EmbyResource 的续播点。
    val resumeMs = (nextEp?.positionTicks ?: 0L) / 10_000L
    val resumeTotalMs = (nextEp?.runtimeMinutes ?: 0L) * 60_000L
    val playLabel = if (resumeMs >= 5_000L) {
        "继续观看 " + com.kivo.cinema.ui.compose.player.formatPlayerTime(resumeMs)
    } else null
    // 317 · 副标题补进度百分比（Rex 的 Viewing Progress 在播放器按钮上的等价信息）。
    // 只在有真实进度且时长可信时加 —— 时长缺失时算不出百分比，宁可不显示也不给错的。
    val progressSuffix = if (resumeMs >= 5_000L && resumeTotalMs > 0L) {
        " · 已看 " + ((resumeMs * 100 / resumeTotalMs).coerceIn(0L, 100L)) + "%"
    } else ""
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
                            playSubtitle = playSubtitle?.let { it + progressSuffix },
                            playLabel = playLabel,
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
                        if (isPreview) {
                            // 336 R2：列表快照阶段片源/季集尚未解析，留白即可（完整详情到达会补上），
                            // 不显示"没有匹配的本地视频"误导用户。
                            Spacer(Modifier.height(140.dp))
                        } else if (media != null) {
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
            if (d != null && !isPreview) {
                detailLowerSections(d, fg, fgSub, tmdbIsTv, content?.loadImage, openPage)
                // D12：本地文件来源的附加事实。这里在 LazyListScope 里（不是 @Composable 上下文），
                // 必须用 item {} 才有 Composable 作用域。
                val localFacts = content?.local
                if (localFacts != null) item(key = "local-extra") {
                    LocalDetailBlocks(localFacts, fg, fgSub, openPage)
                }
            }
        }
        // 336 R2：有 preview 时首帧内容已在，不显示转圈；仅无 preview 的纯等待期转圈。
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