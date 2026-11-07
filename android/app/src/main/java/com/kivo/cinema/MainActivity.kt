package com.kivo.cinema

import android.database.Cursor
import android.media.ThumbnailUtils
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.ui.design.AnimatedGradientBg
import com.kivo.cinema.ui.design.CapsuleNavBar
import com.kivo.cinema.ui.design.DiscoverPage
import com.kivo.cinema.ui.design.ProfilePage
import com.kivo.cinema.ui.design.SettingsPage
import com.kivo.cinema.ui.design.SourcesPage

/**
 * App Shell（236 设计稿 §三）：动画渐变背景 + 4 页路由 + 底部胶囊导航。
 * LAUNCHER（原 MediaLibraryActivity 降为资源库入口的二级页）。
 * 缩略图缓存：ThumbnailUtils 逐条 MINI_KIND + HashMap——v1 明示。
 */
class MainActivity : ComponentActivity() {

    private var items by mutableStateOf(listOf<MediaItem>())
    private val thumbCache = HashMap<Long, android.graphics.Bitmap?>()

    // 237-A：观看数据域 + 评分源 + 首页 TMDB 节
    private val watchStore by lazy { com.kivo.cinema.stats.WatchStore(this) }
    private val tmdb by lazy { com.kivo.cinema.tmdb.TmdbClient(this) }
    private val tmdbHome by lazy { com.kivo.cinema.tmdb.TmdbHome(tmdb) }
    private var heroCandidates by mutableStateOf(
        listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var nowPlaying by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var onTheAir by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var trending by mutableStateOf(listOf<com.kivo.cinema.tmdb.TmdbHome.Item>())
    private var continueItems by mutableStateOf(
        listOf<Pair<MediaItem, com.kivo.cinema.stats.WatchStore.Progress>>())
    private var heroRating by mutableStateOf<Pair<String, Float>?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        com.kivo.cinema.ui.design.TmdbImageCache.diskDir = cacheDir
        com.kivo.cinema.tmdb.TmdbClient.setShared(
            com.kivo.cinema.tmdb.TmdbClient(this))
        scanVideos()
        setContent { Shell() }
    }

    override fun onResume() {
        super.onResume()
        refreshContinue()
        refreshTmdb()
    }

    private fun refreshTmdb() {
        Thread {
            val hero = tmdbHome.heroCandidates()
            val np = tmdbHome.nowPlaying()
            val oa = tmdbHome.onTheAir()
            val tr = tmdbHome.trendingWeek()
            runOnUiThread {
                heroCandidates = hero
                nowPlaying = np
                onTheAir = oa
                trending = tr
            }
        }.start()
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
        var page by remember { mutableStateOf(0) }
        AnimatedGradientBg {
            Column(Modifier.fillMaxSize()) {
                Box(Modifier.weight(1f)) {
                    when (page) {
                        0 -> DiscoverPage(
                            items = items,
                            continueItems = continueItems,
                            heroCandidates = heroCandidates,
                            nowPlaying = nowPlaying,
                            onTheAir = onTheAir,
                            trending = trending,
                            posterOf = { posterBlockOf(it) },
                            onOpenItem = { openPlayer(it) },
                            onOpenLibrary = { openLibrary() },
                            onOpenTmdbItem = { },
                        )
                        1 -> ProfilePage(items.size)
                        2 -> SourcesPage(
                            onOpenSmb = { startActivity(
                                android.content.Intent(this@MainActivity,
                                    com.kivo.cinema.smb.SmbActivity::class.java)) },
                            onOpenEmby = { startActivity(
                                android.content.Intent(this@MainActivity,
                                    com.kivo.cinema.emby.EmbyActivity::class.java)) },
                            onOpenLibrary = { openLibrary() },
                            onOpenPrivate = { startActivity(
                                android.content.Intent(this@MainActivity,
                                    com.kivo.cinema.privatezone.PrivateActivity::class.java)) },
                            videoCount = items.size,
                        )
                        else -> SettingsPage(
                            clearTextOk = applicationInfo.flags and
                                android.content.pm.ApplicationInfo.FLAG_USES_CLEARTEXT_TRAFFIC != 0,
                            buildLabel = "0.1.0 (236-ui)",
                        )
                    }
                }
                // 距底 22pt（帧实测）
                Box(Modifier.padding(bottom = 22.dp)) {
                    CapsuleNavBar(
                        tabs = listOf("发现" to "🎞", "我的" to "👤",
                            "资源库" to "🗄", "设置" to "⚙"),
                        selected = page,
                        onSelect = { page = it },
                        onSearch = { openLibrary() },
                    )
                }
            }
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
