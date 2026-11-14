package com.kivo.cinema

import androidx.lifecycle.lifecycleScope
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

/** 继续观看：取最近 10 条观看记录，与本地片源按路径对齐后回填。 */
internal fun MainActivity.refreshContinue() {
    Thread {
        val pairs = watchStore.recentKeys(10).mapNotNull { key ->
            val prog = watchStore.progressOf(key) ?: return@mapNotNull null
            val nk = normPath(key)
            val item = items.firstOrNull { normPath(it.path) == nk }
                ?: return@mapNotNull null
            item to prog
        }
        runOnUiThread { shellState.continueItems = pairs }
    }.start()
}

/** /sdcard 与 /storage/emulated/0 是同一文件的两个别名——统一后比较。 */
private fun normPath(p: String): String = when {
    p.startsWith("/sdcard/") -> "/storage/emulated/0/" + p.removePrefix("/sdcard/")
    else -> p
}
