package com.kivo.cinema

import android.database.Cursor
import android.media.ThumbnailUtils
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import com.kivo.cinema.ui.design.home.navigationSource
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.ui.design.AnimatedGradientBg
import com.kivo.cinema.ui.design.CapsuleNavBar
import com.kivo.cinema.ui.design.DiscoverPage
import com.kivo.cinema.ui.design.ProfilePage
import com.kivo.cinema.ui.design.SettingsPage
import com.kivo.cinema.sources.ui.SourcesScreen

/**
 * App Shell（236 设计稿 §三）：动画渐变背景 + 4 页路由 + 底部胶囊导航。
 * LAUNCHER（原 MediaLibraryActivity 降为资源库入口的二级页）。
 * 缩略图缓存：ThumbnailUtils 逐条 MINI_KIND + HashMap——v1 明示。
 */
class MainActivity : ComponentActivity() {

    private var items by mutableStateOf(listOf<MediaItem>())
    private var localVideoCount by mutableStateOf<Int?>(null)
    private val thumbCache = HashMap<Long, android.graphics.Bitmap?>()
    private val sources by lazy { androidx.lifecycle.ViewModelProvider(this)[com.kivo.cinema.sources.SourceManagerModel::class.java] }
    private lateinit var sourceUnlock: com.kivo.cinema.sources.access.SourceUnlockController
    private var defaultHomeHandled = false

    // 237-A：观看数据域 + 评分源 + 首页 TMDB 节
    private val watchStore by lazy { com.kivo.cinema.stats.WatchStore(this) }
    private val tmdb by lazy { com.kivo.cinema.tmdb.TmdbClient(this) }
    private val homePreferences by lazy { com.kivo.cinema.ui.design.home.HomePreferences(this) }
    private val tmdbHome by lazy { com.kivo.cinema.tmdb.TmdbHome(tmdb) }
    private var heroCandidates by mutableStateOf(
        listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var nowPlaying by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var onTheAir by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var trending by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var variety by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var todayAnime by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var tmdbError by mutableStateOf<String?>(null)   // home.error.network.* 键尾
    private var homeGeneration by mutableStateOf(0)
    private var tmdbRefreshing = false
    private var continueItems by mutableStateOf(
        listOf<Pair<MediaItem, com.kivo.cinema.stats.WatchStore.Progress>>())
    private var ambientDark by mutableStateOf(false)   // 首页环境暗底（导航玻璃/状态栏联动）

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        com.kivo.cinema.sources.access.SourceAccessSession.install(application)
        sourceUnlock = com.kivo.cinema.sources.access.SourceUnlockController(this)
        defaultHomeHandled = savedInstanceState?.getBoolean("source.defaultHandled") ?: false
        // edge-to-edge（239：hero 顶入状态栏下——windowFullscreen 已从主题移除）
        enableEdgeToEdge()
        if (android.os.Build.VERSION.SDK_INT >= 29) window.isNavigationBarContrastEnforced = false
        window.navigationBarColor = android.graphics.Color.TRANSPARENT
        com.kivo.cinema.ui.design.TmdbImageCache.diskDir = cacheDir
        com.kivo.cinema.tmdb.TmdbClient.setShared(
            com.kivo.cinema.tmdb.TmdbClient(this))
        setContent { Shell() }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putBoolean("source.defaultHandled", defaultHomeHandled)
        super.onSaveInstanceState(outState)
    }

    override fun onResume() {
        super.onResume()
        scanVideos()
        refreshContinue()
        refreshTmdb()
        sources.refresh()
    }

    private fun refreshTmdb() {
        if (tmdbRefreshing) return
        tmdbRefreshing = true
        lifecycleScope.launch {
            try {
                val tr = async(Dispatchers.IO) { tmdbHome.trendingWeek() }
                val np = async(Dispatchers.IO) { tmdbHome.nowPlaying() }
                val oa = async(Dispatchers.IO) { tmdbHome.onTheAir() }
                val va = async(Dispatchers.IO) { tmdbHome.varietyPastMonth() }
                val an = async(Dispatchers.IO) { tmdbHome.todayAnime() }
                val trend = tr.await()
                if (trend.items.isNotEmpty()) {
                    heroCandidates = trend.items.take(10)
                    trending = trend.items
                    // Keep the active page/player stable when a refresh returns identical candidates.
                    // Metadata repositories independently expire their cache after 15 minutes.
                    if (BuildConfig.DEBUG) withContext(Dispatchers.IO) { runCatching {
                        val rows = org.json.JSONArray()
                        heroCandidates.forEach { m -> rows.put(org.json.JSONObject()
                            .put("id", m.id).put("is_tv", m.isTv).put("title", m.title)
                            .put("poster_url", m.posterHeroUrl)) }
                        val file = java.io.File(filesDir, "evidence/home-feed.json")
                        file.parentFile?.mkdirs()
                        file.writeText(org.json.JSONObject().put("fetched_at_epoch_ms", System.currentTimeMillis())
                            .put("source", "/trending/all/week?language=zh-CN")
                            .put("items", rows).toString(2))
                    } }
                }
                val now = np.await(); val air = oa.await()
                val varieties = va.await(); val anime = an.await()
                if (now.errorKind == null) nowPlaying = now.items
                if (air.errorKind == null) onTheAir = air.items
                if (varieties.errorKind == null) variety = varieties.items
                if (anime.errorKind == null) todayAnime = anime.items
                tmdbError = listOf(trend.errorKind, now.errorKind, air.errorKind,
                    varieties.errorKind, anime.errorKind).firstOrNull { it != null }
            } finally {
                tmdbRefreshing = false
            }
        }
    }

    private fun refreshContinue() {
        Thread {
            val pairs = watchStore.recentKeys(10).mapNotNull { key ->
                val prog = watchStore.progressOf(key) ?: return@mapNotNull null
                val nk = normPath(key)
                val item = items.firstOrNull { normPath(it.path) == nk }
                    ?: return@mapNotNull null
                item to prog
            }
            runOnUiThread { continueItems = pairs }
        }.start()
    }

    /** /sdcard 与 /storage/emulated/0 是同一文件的两个别名——统一后比较。 */
    private fun normPath(p: String): String = when {
        p.startsWith("/sdcard/") -> "/storage/emulated/0/" + p.removePrefix("/sdcard/")
        else -> p
    }

    private fun scanVideos() {
        val list = mutableListOf<MediaItem>()
        var readable = false
        val projection = arrayOf(
            MS.Video.Media._ID, MS.Video.Media.DISPLAY_NAME, MS.Video.Media.DATA,
            MS.Video.Media.DURATION, MS.Video.Media.WIDTH, MS.Video.Media.HEIGHT,
            MS.Video.Media.DATE_ADDED,
        )
        runCatching {
            contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI, projection, null, null,
                "${MS.Video.Media.DATE_ADDED} DESC"
            )?.use { c: Cursor ->
                readable = true
                while (c.moveToNext()) {
                    list.add(MediaItem(
                        id = c.getLong(0), title = c.getString(1) ?: "",
                        path = c.getString(2) ?: "", durationMs = c.getLong(3),
                        width = c.getInt(4), height = c.getInt(5),
                        addedSec = c.getLong(6),
                    ))
                }
            }
        }
        items = list
        val privateIds = com.kivo.cinema.privatezone.PrivateStore(this).all()
        localVideoCount = if (readable) list.count { it.id.toString() !in privateIds } else null
    }

    @Composable
    private fun posterBlockOf(item: MediaItem): @Composable () -> Unit {
        val bmp = remember(item.id) {
            thumbCache.getOrPut(item.id) {
                runCatching {
                    ThumbnailUtils.createVideoThumbnail(item.path,
                        MS.Video.Thumbnails.MINI_KIND)
                }.getOrNull()
            }
        }
        return {
            if (bmp != null) {
                Image(bmp.asImageBitmap(), item.title,
                    contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
            } else {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("🎬", fontSize = 26.sp)
                }
            }
        }
    }

    @Composable
    private fun Shell() {
        var page by androidx.compose.runtime.saveable.rememberSaveable { mutableStateOf(1) }
        var sourceNavigationGuard by remember { mutableStateOf<((() -> Unit) -> Unit)?>(null) }
        androidx.compose.runtime.LaunchedEffect(sources.ready) {
            if (sources.ready && !defaultHomeHandled) {
                defaultHomeHandled = true
                sources.records.firstOrNull { it.defaultHome }?.let { source ->
                    page = 0
                    startActivity(com.kivo.cinema.sources.SourceOpen.intentFor(this@MainActivity, source))
                }
            }
        }
        androidx.compose.runtime.SideEffect {
            if (com.kivo.cinema.sources.access.SourceAccessSession.unlocked && sources.records.any { it.hidden })
                window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
            else window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
        }
        var previewEnabled by remember { mutableStateOf(homePreferences.autoplay) }
        var selectedTitle by remember { mutableStateOf<com.kivo.cinema.tmdb.TmdbHome.Item?>(null) }
        val backdrop = remember { com.kivo.cinema.ui.design.home.NavigationBackdropState() }
        val shellNavigation: @Composable () -> Unit = {
                    CapsuleNavBar(
                        tabs = listOf("发现" to "", "我的" to "", "资源库" to "", "设置" to ""),
                        selected = listOf(1, 3, 0, 4).indexOf(page),
                        onSelect = {
                            val next = listOf(1, 3, 0, 4)[it]
                            if (next != page) {
                                if (page == 0 && sourceNavigationGuard != null) sourceNavigationGuard?.invoke { page = next }
                                else page = next
                            }
                        },
                        onSearch = {
                            startActivity(android.content.Intent(this,
                                com.kivo.cinema.emby.aggregate.ui.AggregateActivity::class.java))
                        },
                        onDark = page == 1 && ambientDark,
                        backdrop = backdrop,
                    )
        }
        AnimatedGradientBg {
            // 悬浮导航：内容全屏铺底，胶囊浮在内容上（0082 帧：卡标签被导航遮住）
            Box(Modifier.fillMaxSize()) {
                Box(
                    Modifier
                        .fillMaxSize()
                        // 发现页不裁底：环境层要铺到导航后面（239 §3.2）；其余页留导航空间
                        .padding(bottom = if (page == 0 || page == 1) 0.dp else 84.dp)
                          .windowInsetsPadding(if (page == 0 || page == 1) WindowInsets(0, 0, 0, 0) else WindowInsets.statusBars)
                        .navigationSource(backdrop, enabled = page != 1 && page != 0)
                ) {
                    when (page) {
                        0 -> SourcesScreen(
                            model = sources,
                            navigation = shellNavigation,
                            overviewModifier = Modifier.navigationSource(backdrop),
                            onNavigationGuard = { sourceNavigationGuard = it },
                            onUnlock = { action -> sourceUnlock.authenticate(sources::reportError, action) },
                            onOpenSource = { source ->
                                startActivity(com.kivo.cinema.sources.SourceOpen.intentFor(this@MainActivity, source))
                            },
                            onOpenLibrary = { openLibrary() },
                            onOpenPrivate = { startActivity(
                                android.content.Intent(this@MainActivity,
                                    com.kivo.cinema.privatezone.PrivateActivity::class.java)) },
                            onOpenAggregate = { startActivity(
                                android.content.Intent(this@MainActivity,
                                    com.kivo.cinema.emby.aggregate.ui.AggregateActivity::class.java)) },
                            videoCount = localVideoCount,
                        )
                        1 -> DiscoverPage(
                            backdrop = backdrop,
                            previewEnabled = previewEnabled,
                            continueItems = continueItems,
                            heroCandidates = heroCandidates,
                            nowPlaying = nowPlaying,
                            onTheAir = onTheAir,
                            trending = trending,
                            variety = variety,
                            todayAnime = todayAnime,
                            topPoster = { tmdbHome.topVotedPosterUrl(it) },
                            logoFor = { tmdbHome.logoUrlFor(it) },
                            trailerFor = { tmdbHome.trailerFor(it) },
                            proxyAddress = tmdb.proxyAddr(),
                            generation = homeGeneration,
                            posterOf = { posterBlockOf(it) },
                            onOpenItem = { openPlayer(it) },
                            onOpenTmdbItem = { startActivity(com.kivo.cinema.ui.detail.MovieDetailActivity.intent(this@MainActivity, it)) },
                            tmdbError = tmdbError,
                            onRetryTmdb = { refreshTmdb() },
                            onAmbientDark = { ambientDark = it },
                            onShowWatching = { page = 2 },
                        )
                        2 -> com.kivo.cinema.ui.design.WatchingPage(
                            continueItems = continueItems,
                            posterOf = { posterBlockOf(it) },
                            onOpenItem = { openPlayer(it) },
                        )
                        3 -> ProfilePage(items.size)
                        else -> SettingsPage(
                            previewEnabled = previewEnabled,
                            onPreviewChanged = { previewEnabled = it; homePreferences.autoplay = it },
                            clearTextOk = applicationInfo.flags and
                                android.content.pm.ApplicationInfo.FLAG_USES_CLEARTEXT_TRAFFIC != 0,
                            buildLabel = "0.1.0 (237-ui)",
                        )
                    }
                }
                // 距底 22pt（帧实测）；发现页深色环境时转暗玻璃（E11）
                if (page != 0) Box(Modifier.align(Alignment.BottomCenter).padding(bottom = 22.dp)) {
                    shellNavigation()
                }
            }
        }
        selectedTitle?.let { title ->
            com.kivo.cinema.ui.design.home.HomeTitleSheet(title) { selectedTitle = null }
        }
        // 状态栏图标随环境明暗（E11：暗底 → 白图标）
        val view = findViewById<android.view.View>(android.R.id.content)
        androidx.core.view.WindowCompat.getInsetsController(window, view).apply {
            isAppearanceLightStatusBars = !(page == 1 && ambientDark)
            isAppearanceLightNavigationBars = !(page == 1 && ambientDark)
        }
    }

    private fun openLibrary() {
        startActivity(android.content.Intent(this,
            com.kivo.cinema.library.MediaLibraryActivity::class.java))
    }

    private fun openPlayer(item: MediaItem) {
        startActivity(android.content.Intent(this,
            com.kivo.cinema.ui.detail.DetailActivity::class.java)
            .putExtra("kivo.media", item.path)
            .putExtra("kivo.durationMs", item.durationMs))
    }
}
