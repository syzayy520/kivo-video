package com.kivo.cinema

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.home.HomePreferences

/**
 * App Shell 宿主（236 设计稿 §三）：动画渐变背景 + 4 页路由 + 底部胶囊导航。
 * LAUNCHER（原 MediaLibraryActivity 降为资源库入口的二级页）。
 *
 * 本类只留**状态所有权与生命周期**：首页数据供给见 [refreshHomeFeed]，
 * 本地媒体扫描见 [scanLocalVideos]，路由与页面编排见 [MainShell]。
 */
class MainActivity : ComponentActivity() {

    /** 本地视频列表（资源库/我的/海报块共用一份）。 */
    internal var items by mutableStateOf(listOf<MediaItem>())

    /** 首页 Shell 的数据域（TMDB 分区 + 继续观看 + 环境暗底 + 本地可见数）。 */
    internal val shellState = HomeShellState()

    /** 本地媒体索引（MediaStore 扫描 + 缩略图缓存）。 */
    internal val localMedia by lazy { LocalMediaIndex(this) }

    internal val sources by lazy {
        androidx.lifecycle.ViewModelProvider(this)[com.kivo.cinema.sources.SourceManagerModel::class.java]
    }
    /** 默认首页已处理（一次性动作；跨重建靠 savedInstanceState 记）。 */
    internal var defaultHomeHandled = false

    // 312 · 品牌启动动画
    /** 策略与计划（"已播完"的状态在 Compose 里用 remember 持有）。 */
    private val startupPolicy by lazy { com.kivo.cinema.ui.brand.StartupPolicy(this) }
    private val startupPlan by lazy {
        startupPolicy.planForColdStart(System.currentTimeMillis())
    }

    // 237-A：观看数据域 + 评分源 + 首页 TMDB 节
    internal val watchStore by lazy { WatchStore(this) }
    internal val tmdb by lazy { TmdbClient(this) }
    internal val homePreferences by lazy { HomePreferences(this) }
    internal val tmdbHome by lazy { TmdbHome(tmdb) }
    /** TMDB 抓取进行中：同一时刻只跑一轮。 */
    internal var tmdbRefreshing = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        com.kivo.cinema.net.NetDiag.attach(application)
        com.kivo.cinema.sources.access.SourceAccessSession.install(application)
        // D10：模块开关在启动期读盘一次，进入「我的」页无闪烁、组合期无磁盘 IO。
        com.kivo.cinema.ui.profile.modules.ProfileModulePrefs.bind(this)
        defaultHomeHandled = savedInstanceState?.getBoolean("source.defaultHandled") ?: false
        // edge-to-edge（239：hero 顶入状态栏下——windowFullscreen 已从主题移除）
        enableEdgeToEdge()
        if (android.os.Build.VERSION.SDK_INT >= 29) window.isNavigationBarContrastEnforced = false
        window.navigationBarColor = android.graphics.Color.TRANSPARENT
        com.kivo.cinema.ui.design.TmdbImageCache.diskDir = cacheDir
        TmdbClient.setShared(TmdbClient(this))
        // 312 · 品牌启动动画：只有**冷启动**（savedInstanceState == null）才播，
        // 进程被系统杀掉再恢复（savedInstanceState 非 null）不重播。
        val coldStart = savedInstanceState == null
        setContent {
            MainShell(this, shellState)
            // 【必须用 Compose State】之前用 Activity 普通字段存"已播完"，
            // onFinished 里改它**不会触发重组** ⇒ 覆盖层永远留在界面上，
            // 真机上表现为主页中间残留一条细线（动画停在 t≈0.4 s 的那一帧）。
            var brandDone by remember { mutableStateOf(false) }
            if (coldStart && !brandDone) {
                com.kivo.cinema.ui.brand.StartupOverlay(
                    plan = startupPlan,
                    onFinished = {
                        startupPolicy.markFullShown(System.currentTimeMillis())
                        startupPolicy.markVersionSeen()
                        brandDone = true
                    },
                )
            }
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putBoolean("source.defaultHandled", defaultHomeHandled)
        super.onSaveInstanceState(outState)
    }

    override fun onResume() {
        super.onResume()
        scanLocalVideos()
        refreshContinue()
        refreshHomeFeed()
        sources.refresh()
    }

    internal fun openLibrary() {
        startActivity(android.content.Intent(this,
            com.kivo.cinema.library.MediaLibraryActivity::class.java))
    }

    internal fun openPlayer(item: MediaItem) {
        // D12：本地文件也走统一详情页（path 即片源，TMDB 只做增强）。
        startActivity(com.kivo.cinema.ui.detail.MovieDetailActivity.localIntent(this, item))
    }

    /**
     * 首页「继续观看」跨源条目打开：本地/SMB 走统一详情，Emby 进对应服务器页。
     *
     * Emby 深链详情需要 token（`MovieDetailActivity.embyIntent` 要 token/userId），
     * 而凭据刻意不进 [com.kivo.cinema.library.HomeEntry] ⇒ P1 先落到 Emby 页，
     * 用户在服务器页里再点具体片；后续要直达详情时补一条"按 sourceId 取会话"的入口。
     */
    internal fun openHomeEntry(entry: com.kivo.cinema.library.HomeEntry) {
        when (entry.kind) {
            com.kivo.cinema.library.HomeEntry.Kind.Emby -> startActivity(
                android.content.Intent(this, com.kivo.cinema.emby.EmbyActivity::class.java)
                    .putExtra("kivo.sourceId", entry.embySourceId),
            )
            com.kivo.cinema.library.HomeEntry.Kind.Smb -> {
                val p = com.kivo.cinema.library.SmbContinueKey.parse(entry.key) ?: return
                startActivity(
                    android.content.Intent(this,
                        com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                        .putExtra("kivo.sourceId", p.sourceId)
                        .putExtra("kivo.smbShare", p.share)
                        .putExtra("kivo.smbPath", p.path)
                        .putExtra("kivo.label", p.fileName),
                )
            }
            else -> entry.mediaItem?.let { openPlayer(it) }
        }
    }
}
