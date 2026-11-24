package com.kivo.cinema

import android.content.Intent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.privatezone.PrivateActivity
import com.kivo.cinema.sources.SourceOpen

import com.kivo.cinema.sources.ui.SourcesScreen
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.AnimatedGradientBg
import com.kivo.cinema.ui.design.CapsuleNavBar
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.WatchingPage
import com.kivo.cinema.ui.design.home.HomeTitleSheet
import com.kivo.cinema.ui.design.settings.SettingsPage
import com.kivo.cinema.ui.profile.ProfilePage
import com.kivo.cinema.ui.profile.ProfileStats
import com.kivo.cinema.ui.profile.model.ProfileDashboard
import com.kivo.cinema.ui.shell.ShellInsets
import com.kivo.cinema.ui.shell.ShellTab
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

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
    var continueItems by mutableStateOf(listOf<HomeEntry>())
    var homeEntries by mutableStateOf(listOf<HomeEntry>())   // 首页=NAS+Emby（排除手机本地视频）
    var nasSections by mutableStateOf(listOf<com.kivo.cinema.library.NasLibrarySource.Section>())  // 首页本地片库分类行
    var ambientDark by mutableStateOf(false)                 // 首页环境暗底（导航玻璃/状态栏联动）
    var localVideoCount by mutableStateOf<Int?>(null)        // 本地可见视频数；不可读为 null
}

/**
 * App Shell 的路由编排（从 MainActivity 抽出，控行数）：4 页切换 + 胶囊导航 + 浮层。
 * 唯一职责 = 把 [HomeShellState] 与 Activity 的能力接成页面树；数据供给不在这里。
 */
@Composable
internal fun MainShell(activity: MainActivity, state: HomeShellState) {
    var page by rememberSaveable { mutableStateOf(ShellTab.Discover.page) }
    // 「在看」是「我的」模块的二级页，不再是第 5 个 Tab 路由（B2 / SHELL-02）：
    // 覆盖层状态独立于 page，返回即关闭，不改变底部 Tab 位置。
    var showWatching by rememberSaveable { mutableStateOf(false) }
    // 「搜索」是一级入口（底栏放大镜）：全屏覆盖层，①③④ 带底栏、② 输入中不带。
    var showSearch by rememberSaveable { mutableStateOf(false) }
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

    var previewEnabled by remember { mutableStateOf(activity.homePreferences.autoplay) }
    var selectedTitle by remember { mutableStateOf<TmdbHome.Item?>(null) }
    val shellNavigation: @Composable () -> Unit = {
        CapsuleNavBar(
            tabs = ShellTab.displayOrder.map { it.label to "" },
            // 胶囊下标 = displayOrder 下标，不是 entries.ordinal（两者不同序）。
            // 「在看」已迁出路由（B2 覆盖层），不再需要把选中态借给「发现」。
            selected = ShellTab.displayOrder.indexOfFirst { it.page == page }.coerceAtLeast(0),
            onSelect = {
                val next = ShellTab.displayOrder[it].page
                if (next != page) {
                    if (page == 0 && sourceNavigationGuard != null) sourceNavigationGuard?.invoke { page = next }
                    else page = next
                }
            },
            onSearch = { showSearch = true },
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
                    // 发现页/资源库页不裁底：环境层与资源库面板要铺到导航后面（239 §3.2）；
                    // 「我的」页在 ProfilePage 内部按 ShellInsets 预留（避免双层空白）；
                    // 其余页由 Shell 预留导航高度（不再写死 84.dp）。
                    .padding(
                        bottom = if (page == ShellTab.Discover.page || page == ShellTab.Library.page
                            || page == ShellTab.Profile.page) 0.dp
                        else ShellInsets.NavReservedHeight,
                    )
                    .windowInsetsPadding(
                        if (page == ShellTab.Discover.page || page == ShellTab.Library.page) WindowInsets(0, 0, 0, 0)
                        else WindowInsets.statusBars,
                    )
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
                        videoCount = state.localVideoCount,
                    )
                    1 -> DiscoverTab(activity, state, previewEnabled) { showWatching = true }
                    3 -> {
                        // 看板读取磁盘 + MediaStore（阻塞），放 IO；「我的」页自身预留导航高度，
                        // 所以这个分支不需要 Shell 兜底 padding。
                        val context = androidx.compose.ui.platform.LocalContext.current
                        // PROF-03：看板复用 Shell 已对齐的 continueItems（与发现页/在看覆盖层同源），
                        // 不另算一份；它变化时（onResume 刷新）这里会重建快照。
                        val dashboard by produceState<ProfileDashboard?>(
                            initialValue = null, context, state.continueItems,
                        ) {
                            value = withContext(Dispatchers.IO) {
                                ProfileStats.build(context, state.continueItems)
                            }
                        }
                        val ready = dashboard
                        if (ready == null) Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                            Text("正在统计观看记录…", fontSize = 13.sp, color = DesignTokens.textSub)
                        } else ProfilePage(
                            dashboard = ready,
                            onEditEnvironment = {}, // B5 接环境编辑
                            onOpenWatching = { showWatching = true },
                            onManageModules = {}, // 模块 sheet 由页面内底部按钮打开
                            onOpenItem = {}, // B5 接详情
                        )
                    }
                    else -> SettingsPage(
                        previewEnabled = previewEnabled,
                        onPreviewChanged = { previewEnabled = it; activity.homePreferences.autoplay = it },
                        onOpenSources = { page = 0 },
                        onOpenPrivate = { activity.startActivity(Intent(activity, PrivateActivity::class.java)) },
                        onCategoryChanged = { activity.refreshNasLibrary() },
                    )
                }
            }
            // 距底 22pt（帧实测）；发现页深色环境时转暗扁平（E11，269）
            // 导航条要避让系统手势区：edge-to-edge 下不避让会被手势条压住（272 审计 D7）。
            // 「在看」覆盖层是全屏二级页，期间不渲染胶囊（否则点击会穿透到 Tab 切换）。
            if (page != 0 && !showWatching && !showSearch) Box(Modifier.align(Alignment.BottomCenter)
                .navigationBarsPadding().padding(bottom = 22.dp)) {
                shellNavigation()
            }
            // 「在看」二级页：不透明铺底盖住 Tab 内容，返回（导航条 / 系统返回键）关闭覆盖层。
            if (showWatching) {
                Box(
                    Modifier.fillMaxSize()
                        .background(DesignTokens.pageBackground)
                        .padding(bottom = ShellInsets.NavReservedHeight)
                        .windowInsetsPadding(WindowInsets.statusBars)
                ) {
                    WatchingPage(
                        continueItems = state.continueItems,
                        posterOf = { activity.entryPoster(it) }, onOpenItem = { activity.openHomeEntry(it) },
                        onRemove = { activity.removeWatching(it) },
                        onBack = { showWatching = false },
                    )
                }
            }
        }
    }
            // 「搜索」覆盖层：scaffold 托管在 MainActivitySearch.kt。
            if (showSearch) activity.SearchOverlayScaffold(
                onBack = { showSearch = false },
                onSelectTab = { showSearch = false; page = it },
            )
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
