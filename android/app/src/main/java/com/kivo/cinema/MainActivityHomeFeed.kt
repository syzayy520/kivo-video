package com.kivo.cinema

import androidx.compose.runtime.Composable
import androidx.lifecycle.lifecycleScope
import com.kivo.cinema.emby.EmbyHomeResume
import com.kivo.cinema.library.ContinueWatching
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.HomeFeedAggregator
import com.kivo.cinema.library.SmbContinueSource
import com.kivo.cinema.ui.design.home.sections.EmbyResumePoster
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 首页数据供给（从 MainActivity 抽出，控行数）：TMDB 分区抓取 + 继续观看。
 * 唯一职责 = 把数据填进 [HomeShellState]；不做 UI，也不碰路由。
 */

/** TMDB 五分区抓取：一次刷新喂首页 hero + 四个分区行，失败只报第一个错误键尾。 */
internal fun MainActivity.refreshHomeFeed() {
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
                shellState.heroCandidates = trend.items.take(10)
                shellState.trending = trend.items
                // Keep the active page/player stable when a refresh returns identical candidates.
                // Metadata repositories independently expire their cache after 15 minutes.
                if (BuildConfig.DEBUG) withContext(Dispatchers.IO) { runCatching {
                    val rows = org.json.JSONArray()
                    shellState.heroCandidates.forEach { m -> rows.put(org.json.JSONObject()
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
            if (now.errorKind == null) shellState.nowPlaying = now.items
            if (air.errorKind == null) shellState.onTheAir = air.items
            if (varieties.errorKind == null) shellState.variety = varieties.items
            if (anime.errorKind == null) shellState.todayAnime = anime.items
            shellState.tmdbError = listOf(trend.errorKind, now.errorKind, air.errorKind,
                varieties.errorKind, anime.errorKind).firstOrNull { it != null }
        } finally {
            tmdbRefreshing = false
        }
    }
}

/**
 * 继续观看：首页与「在看」的范围**不同**，但「在看」必须是全量。
 *
 * - 首页 `homeEntries` = NAS(SMB) + Emby：手机本地视频不可刮削，混进首页会把版面拉丑，**必须排除**；
 * - 在看页 `continueItems` = **本地 + NAS(SMB) + Emby**：它是"我的进度"总账，
 *   播过的片（尤其 NAS）必须在这里看得到。早先只填本地，导致"首页有、在看没有"。
 *
 * 两段式取数，避免网络往返拖住本地：本地先算先出；SMB 读续播键、Emby 后台补拉，
 * 到了再刷新首页。任何一源失败静默降级——首页不因某源不可达而报错或白屏。
 */
private val continueBusy = java.util.concurrent.atomic.AtomicBoolean(false)

internal fun MainActivity.refreshContinue() {
    // onResume 在冷启动期可能被连续触发多次；不防重入会重复登录 Emby、重复拉续播（真机实测 3 次）。
    if (!continueBusy.compareAndSet(false, true)) return
    com.kivo.cinema.net.NetDiag.log("[home] continue 进入")
    Thread {
        try {
        // 本地手机视频：只喂「在看」页，不进首页（用户裁决：不可刮削，会拉丑首页）。
        val local = ContinueWatching.aligned(items, watchStore)
        runOnUiThread { shellState.continueItems = HomeFeedAggregator.merge(local, emptyList()) }
        // 首页：只聚合 NAS(SMB) + Emby。
        val smb = SmbContinueSource.recent(watchStore)
        com.kivo.cinema.net.NetDiag.log("[home] smb=${smb.size}")
        // SMB 先到先显示：别为等 Emby 网络把整行憋着不渲染。
        if (smb.isNotEmpty()) {
            val onlySmb = HomeFeedAggregator.merge(emptyList(), smb)
            runOnUiThread {
                shellState.homeEntries = onlySmb
                // 在看页是全量账本：SMB 到达即补进去，否则"首页有、在看没有"。
                shellState.continueItems = HomeFeedAggregator.merge(local, smb)
            }
        }
        // Emby 先到先显示：一个源回来就刷新一次，不等最慢/卡死的源。
        var last: List<HomeEntry> = emptyList()
        val emby = EmbyHomeResume.load(this) { partial ->
            val home = HomeFeedAggregator.merge(emptyList(), smb + partial)
            if (home != last) {
                last = home
                com.kivo.cinema.net.NetDiag.log("[home] 增量 homeEntries=${home.size}")
                runOnUiThread {
                    shellState.homeEntries = home
                    shellState.continueItems = HomeFeedAggregator.merge(local, smb + partial)
                }
            }
        }
        com.kivo.cinema.net.NetDiag.log("[home] emby=${emby.size}")
            val home = HomeFeedAggregator.merge(emptyList(), smb + emby)
            com.kivo.cinema.net.NetDiag.log("[home] homeEntries=${home.size}")
            runOnUiThread {
                shellState.homeEntries = home
                shellState.continueItems = HomeFeedAggregator.merge(local, smb + emby)
            }
        } catch (t: Throwable) {
            // 静默早退曾让「继续观看」整行消失且零线索；任何异常都必须留痕，然后仍降级为空。
            com.kivo.cinema.net.NetDiag.log("[home] continue 异常 ${t::class.java.simpleName}: ${t.message}")
        } finally {
            continueBusy.set(false)
        }
    }.start()
}

/**
 * 首页条目海报块：Emby 走服务器远程图，本地/SMB 走 MediaStore 抽帧。
 * 抽成扩展是为了让 `MainActivityShell` 的两处调用各只占一行（该文件卡 300 行硬线）。
 */
@Composable
internal fun MainActivity.entryPoster(entry: HomeEntry): (@Composable () -> Unit)? {
    val media = entry.mediaItem
    return when {
        entry.posterUrl != null -> { { EmbyResumePoster(entry) } }
        media != null -> localMedia.posterBlock(media)
        else -> { { com.kivo.cinema.ui.design.home.sections.NasPoster(entry) } }
    }
}

/** NAS 索引有效期：一天。过期才重扫，避免每次进首页都爬 12T。 */
private const val NAS_INDEX_TTL_MS = 24L * 60 * 60 * 1000

/**
 * 首页「本地片库」：**读索引为主**，索引缺失或过期才后台全扫。
 *
 * 为什么不一进来就扫：12T 上千个目录，SMB 逐层列目录是分钟级；挂在首屏上就是把首页拖死。
 * 首次（无索引）会扫一次，之后一天内直接读索引。
 */
internal fun MainActivity.refreshNasLibrary() {
    Thread {
        val store = com.kivo.cinema.library.NasIndexStore(this)
        // 源被删/隐藏后，索引里的旧条目要作废（口径见 NasLibraryIndex，与「查看全部」页共用）。
        var videos = com.kivo.cinema.library.NasLibraryIndex.aliveVideos(this, store)
        val stale = System.currentTimeMillis() - store.scannedAtMs() > NAS_INDEX_TTL_MS
        if (videos.isEmpty() || stale) {
            val scanned = com.kivo.cinema.library.NasScanService.scanAll(this)
            if (scanned.isNotEmpty()) {
                store.write(scanned)
                videos = scanned
            }
        }
        val prefs = com.kivo.cinema.ui.design.home.HomePreferences(this)
        val sections = com.kivo.cinema.library.NasLibrarySource.sections(videos) { prefs.categoryVisible(it) }
        runOnUiThread { shellState.nasSections = sections }
    }.start()
}
