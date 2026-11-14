package com.kivo.cinema

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
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
        defaultHomeHandled = savedInstanceState?.getBoolean("source.defaultHandled") ?: false
        // edge-to-edge（239：hero 顶入状态栏下——windowFullscreen 已从主题移除）
        enableEdgeToEdge()
        if (android.os.Build.VERSION.SDK_INT >= 29) window.isNavigationBarContrastEnforced = false
        window.navigationBarColor = android.graphics.Color.TRANSPARENT
        com.kivo.cinema.ui.design.TmdbImageCache.diskDir = cacheDir
        TmdbClient.setShared(TmdbClient(this))
        setContent { MainShell(this, shellState) }
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
        startActivity(android.content.Intent(this,
            com.kivo.cinema.ui.detail.DetailActivity::class.java)
            .putExtra("kivo.media", item.path)
            .putExtra("kivo.durationMs", item.durationMs))
    }
}
