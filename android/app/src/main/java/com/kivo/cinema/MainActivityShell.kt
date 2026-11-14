package com.kivo.cinema

import android.content.Intent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.emby.aggregate.ui.AggregateActivity
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.privatezone.PrivateActivity
import com.kivo.cinema.sources.SourceOpen
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.ui.SourcesScreen
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.AnimatedGradientBg
import com.kivo.cinema.ui.design.CapsuleNavBar
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.DiscoverPage
import com.kivo.cinema.ui.design.ProfilePage
import com.kivo.cinema.ui.design.WatchingPage
import com.kivo.cinema.ui.design.home.HomeTitleSheet
import com.kivo.cinema.ui.design.settings.SettingsPage
import com.kivo.cinema.ui.detail.MovieDetailActivity

/**
 * 首页 Shell 的数据域：TMDB 分区 + 继续观看 + 环境暗底 + 本地可见数。
 * 单独成类是为了让 MainActivity 只做状态所有权、[MainShell] 只做页面编排。
 */
internal class HomeShellState {
    var heroCandidates by mutableStateOf(listOf<TmdbHome.Item>())
    var nowPlaying by mutableStateOf(listOf<TmdbHome.Item>())
    var onTheAir by mutableStateOf(listOf<TmdbHome.Item>())
    var trending by mutableStateOf(listOf<TmdbHome.Item>())
    var variety by mutableStateOf(listOf<TmdbHome.Item>())
    var todayAnime by mutableStateOf(listOf<TmdbHome.Item>())
    /** home.error.network.* 键尾。 */
    var tmdbError by mutableStateOf<String?>(null)
    var homeGeneration by mutableStateOf(0)
    var continueItems by mutableStateOf(listOf<Pair<MediaItem, WatchStore.Progress>>())
    /** 首页环境暗底（导航玻璃/状态栏联动）。 */
    var ambientDark by mutableStateOf(false)
    /** 本地可见视频数；媒体库不可读时为 null。 */
    var localVideoCount by mutableStateOf<Int?>(null)
}

/**
 * App Shell 的路由编排（从 MainActivity 抽出，控行数）：4 页切换 + 胶囊导航 + 浮层。
 * 唯一职责 = 把 [HomeShellState] 与 Activity 的能力接成页面树；数据供给不在这里。
 */
@Composable
internal fun MainShell(activity: MainActivity, state: HomeShellState) {
    var page by rememberSaveable { mutableStateOf(1) }
    var sourceNavigationGuard by remember { mutableStateOf<((() -> Unit) -> Unit)?>(null) }
    var showUnlock by remember { mutableStateOf(false) }
    var unlockAction by remember { mutableStateOf<(() -> Unit)?>(null) }
    LaunchedEffect(activity.sources.ready) {
        if (activity.sources.ready && !activity.defaultHomeHandled) {
            activity.defaultHomeHandled = true
            activity.sources.records.firstOrNull { it.defaultHome }?.let { source ->
                page = 0
                activity.startActivity(SourceOpen.intentFor(activity, source))
            }
        }
    }
    SideEffect {
        if (SourceAccessSession.unlocked && activity.sources.records.any { it.hidden })
            activity.window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
        else activity.window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
    }
    var previewEnabled by remember { mutableStateOf(activity.homePreferences.autoplay) }
    var selectedTitle by remember { mutableStateOf<TmdbHome.Item?>(null) }
    val shellNavigation: @Composable () -> Unit = {
        CapsuleNavBar(
            tabs = listOf("发现" to "", "我的" to "", "资源库" to "", "设置" to ""),
            // 「在看」是发现的子页：选中态归到"发现"，不留空选中的胶囊（272 审计 D5）。
            selected = if (page == 2) 0 else listOf(1, 3, 0, 4).indexOf(page),
            onSelect = {
                val next = listOf(1, 3, 0, 4)[it]
                if (next != page) {
                    if (page == 0 && sourceNavigationGuard != null) sourceNavigationGuard?.invoke { page = next }
                    else page = next
                }
            },
            onSearch = {
                activity.startActivity(Intent(activity, AggregateActivity::class.java))
            },
            onDark = page == 1 && state.ambientDark,
        )
    }
    AnimatedGradientBg(
        // 只有「发现」保留氛围底（它自己画满屏色场，渐变实际不可见）；
        // 其余页一律冷灰扁平底，避免粉紫渐变从「在看」页漏出来（272 审计 D6）。
        flatColor = if (page == 1) null else DesignTokens.pageBackground,
    ) {
        // 悬浮导航：内容全屏铺底，胶囊浮在内容上（0082 帧：卡标签被导航遮住）
        Box(Modifier.fillMaxSize()) {
            Box(
                Modifier
                    .fillMaxSize()
                    // 发现页不裁底：环境层要铺到导航后面（239 §3.2）；其余页留导航空间
                    .padding(bottom = if (page == 0 || page == 1) 0.dp else 84.dp)
                    .windowInsetsPadding(if (page == 0 || page == 1) WindowInsets(0, 0, 0, 0) else WindowInsets.statusBars)
            ) {
                when (page) {
                    0 -> SourcesScreen(
                        model = activity.sources,
                        navigation = shellNavigation,
                        onNavigationGuard = { sourceNavigationGuard = it },
                        onUnlock = { action -> unlockAction = action; showUnlock = true },
                        onOpenSource = { source ->
                            activity.startActivity(SourceOpen.intentFor(activity, source))
                        },
                        onOpenLibrary = { activity.openLibrary() },
                        onOpenPrivate = { activity.startActivity(
                            Intent(activity, PrivateActivity::class.java)) },
                        onOpenAggregate = { activity.startActivity(
                            Intent(activity, AggregateActivity::class.java)) },
                        videoCount = state.localVideoCount,
                    )
                    1 -> DiscoverPage(
                        previewEnabled = previewEnabled,
                        continueItems = state.continueItems,
                        heroCandidates = state.heroCandidates,
                        nowPlaying = state.nowPlaying,
                        onTheAir = state.onTheAir,
                        trending = state.trending,
                        variety = state.variety,
                        todayAnime = state.todayAnime,
                        topPoster = { activity.tmdbHome.topVotedPosterUrl(it) },
                        logoFor = { activity.tmdbHome.logoUrlFor(it) },
                        trailerFor = { activity.tmdbHome.trailerFor(it) },
                        proxyAddress = activity.tmdb.proxyAddr(),
                        generation = state.homeGeneration,
                        posterOf = { activity.localMedia.posterBlock(it) },
                        onOpenItem = { activity.openPlayer(it) },
                        onOpenTmdbItem = { activity.startActivity(MovieDetailActivity.intent(activity, it)) },
                        tmdbError = state.tmdbError,
                        onRetryTmdb = { activity.refreshHomeFeed() },
                        onAmbientDark = { state.ambientDark = it },
                        onShowWatching = { page = 2 },
                    )
                    2 -> WatchingPage(
                        continueItems = state.continueItems,
                        posterOf = { activity.localMedia.posterBlock(it) },
                        onOpenItem = { activity.openPlayer(it) },
                        onBack = { page = 1 },
                    )
                    3 -> ProfilePage(activity.items.size)
                    else -> SettingsPage(
                        previewEnabled = previewEnabled,
                        onPreviewChanged = { previewEnabled = it; activity.homePreferences.autoplay = it },
                        onOpenSources = { page = 0 },
                        onOpenPrivate = { activity.startActivity(
                            Intent(activity, PrivateActivity::class.java)) },
                    )
                }
            }
            // 距底 22pt（帧实测）；发现页深色环境时转暗扁平（E11，269）
            // 导航条要避让系统手势区：edge-to-edge 下不避让会被手势条压住（272 审计 D7）。
            if (page != 0) Box(Modifier.align(Alignment.BottomCenter)
                .navigationBarsPadding().padding(bottom = 22.dp)) {
                shellNavigation()
            }
        }
    }
    selectedTitle?.let { title ->
        HomeTitleSheet(title) { selectedTitle = null }
    }
    if (showUnlock) com.kivo.cinema.privacy.PrivacyUnlockDialog(
        onDismiss = { showUnlock = false; unlockAction = null },
        onUnlocked = { showUnlock = false; val a = unlockAction; unlockAction = null; a?.invoke() },
    )
    // 状态栏图标随环境明暗（E11：暗底 → 白图标）
    val view = activity.findViewById<android.view.View>(android.R.id.content)
    androidx.core.view.WindowCompat.getInsetsController(activity.window, view).apply {
        isAppearanceLightStatusBars = !(page == 1 && state.ambientDark)
        isAppearanceLightNavigationBars = !(page == 1 && state.ambientDark)
    }
}
