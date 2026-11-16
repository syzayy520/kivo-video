package com.kivo.cinema.emby

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Handler
import android.os.Looper
import android.util.LruCache
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import java.net.URI
import java.util.concurrent.Executors
import java.util.concurrent.Future

/** Memory-only session; never persist passwords or tokens. UI writes run on main. */
class EmbySessionModel : ViewModel() {
    private var server by mutableStateOf("")
    private var username by mutableStateOf("")
    private var password by mutableStateOf("")
    var error by mutableStateOf<String?>(null); private set
    var connecting by mutableStateOf(false); private set
    var inLibrary by mutableStateOf(false); private set
    val library = com.kivo.cinema.emby.library.EmbyLibraryModel(::friendlyError)
    var views by mutableStateOf<List<EmbyCatalog.View>>(emptyList()); private set
    var viewsLoaded by mutableStateOf(false); private set
    var drilled by mutableStateOf<EmbyCatalog.View?>(null); private set
    private var client: EmbyClient? = null
    private var auth: EmbyClient.Auth? = null
    private val main = Handler(Looper.getMainLooper())
    private val pool = Executors.newFixedThreadPool(4)  // 337：login 后并发 items+hero+resume
    // 337：login 后并发预取的首屏数据（只依赖 token，不等 items）。
    // 带 generation 标签，切服/重连后旧结果自动失效；@Volatile 保证 UI 线程读到最新。
    @Volatile private var heroPrefetch: HeroPrefetch? = null
    private class HeroPrefetch(val generation: Int, val heroes: List<EmbyHeroItem>,
                               val resumed: List<EmbyHeroItem>)
    private var work: Future<*>? = null
    private var generation = 0
    private val posters = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
        override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
    }
    // 335：改为 state —— serverName 后台异步刷新时要触发顶栏重组。
    private var savedName by mutableStateOf<String?>(null)
    private var savedStarted = false
    fun openSaved(context: android.content.Context, id: String, retry: Boolean = false) {
        if (savedStarted && !retry) return
        savedStarted = true
        error = null; connecting = true
        val ticket = ++generation
        work = pool.submit {
            val result = runCatching {
                val records = com.kivo.cinema.sources.SourceRepository(
                    com.kivo.cinema.sources.storage.SourceStore(context.applicationContext)).read().getOrThrow()
                records.firstOrNull { it.id == id }
                    ?: throw com.kivo.cinema.sources.SourceFailure("此连接已被移除，请返回资源库")
            }
            main.post {
                if (ticket != generation) return@post
                connecting = false
                result.fold({ source ->
                    if (!com.kivo.cinema.sources.access.SourceEligibility.visible(source,
                            com.kivo.cinema.sources.access.SourceAccessSession.unlocked)) {
                        error = "请先解锁隐藏资源库"; savedStarted = false
                        return@fold
                    }
                    server = source.address; username = source.username; password = source.password
                    savedName = source.name
                    connect()
                }, { error = if (it is com.kivo.cinema.sources.SourceFailure) it.reason else "无法读取连接，请返回资源库后重试" })
            }
        }
    }
    val serverName: String get() = savedName ?: runCatching {
        URI(if (server.contains("://")) server.trim() else "https://${server.trim()}").host
    }.getOrNull() ?: "Emby"

    private fun connect() {
        if (connecting || server.isBlank() || username.isBlank()) return
        error = null
        connecting = true
        val ticket = ++generation
        val address = server.trim()
        val user = username.trim()
        val secret = password
        work = pool.submit {
            val result = runCatching {
                val c = EmbyClient(address)
                // 337：连接分段计时（login / items），定位"连接资源库慢"卡在哪一步。
                val t0 = System.currentTimeMillis()
                val a = c.login(user, secret)
                val t1 = System.currentTimeMillis()
                com.kivo.cinema.net.NetDiag.log("[emby-connect] login ${t1 - t0}ms")
                // 337：login 拿到 token 后立即并发预取 hero/resume —— 它们只依赖 token，
                // 不必等 items。这样 hero 的 ~2.1s 与 items 的 ~0.68s 重叠，首屏空白期大幅缩短。
                val recentFuture = pool.submit(java.util.concurrent.Callable<List<EmbyHeroItem>> {
                    runCatching { EmbyHeroSource(c).recent(a, limit = 8) }.getOrDefault(emptyList())
                })
                val resumeFuture = pool.submit(java.util.concurrent.Callable<List<EmbyHeroItem>> {
                    runCatching { EmbyHeroSource(c).resume(a, limit = 12) }.getOrDefault(emptyList())
                })
                val list = c.items(a)
                val t2 = System.currentTimeMillis()
                com.kivo.cinema.net.NetDiag.log("[emby-connect] items ${t2 - t1}ms total ${t2 - t0}ms count=${list.items.size}")
                // 等 hero 预取完成再进首页：缓存已就绪，首页 heroRaw/resumeItems 秒返。
                // recent/resume 与 items 三者并行，这里只是汇总，不额外增加总时长。
                val hs = runCatching { recentFuture.get() }.getOrDefault(emptyList())
                val rs = runCatching { resumeFuture.get() }.getOrDefault(emptyList())
                heroPrefetch = HeroPrefetch(ticket, hs, rs)
                Triple(c, a, list)
            }
            main.post {
                if (ticket != generation) return@post
                connecting = false
                result.fold(onSuccess = { (c, a, list) ->
                    // 335：login+items 成功即显示内容，不再把 ServerName 网络查询串在前面。
                    // 顶栏此时用 openSaved 写入的源记录名（savedName 已有值），无空白。
                    client = c; auth = a; library.attach(c, a, list)
                    inLibrary = true
                    loadViews(c, a)
                    // 服务器自有名字（nyamedia 等）后台补查，成功才覆盖源名；失败保留源名并留日志。
                    pool.submit {
                        val pretty = runCatching { c.publicServerName() }.getOrNull()
                        if (pretty == null) {
                            com.kivo.cinema.net.NetDiag.log("[emby] ServerName 查询失败，保留源名 $savedName")
                        } else {
                            main.post { if (ticket == generation) savedName = pretty }
                        }
                    }
                }, onFailure = { error = friendlyError(it) })
            }
        }
    }

    fun cancel() { ++generation; work?.cancel(true); connecting = false }

    /** 255：服务器自有分类（首页横排）。失败静默为空——首页仍可钻取/搜索，不假装有分类。 */
    private fun loadViews(c: EmbyClient, a: EmbyClient.Auth) {
        val ticket = generation
        pool.submit {
            val r = runCatching {
                val cat = EmbyCatalog(c)
                cat.views(a) + listOfNotNull(cat.varietyView(a))
            }
            main.post { if (ticket == generation) { viewsLoaded = true; r.onSuccess { views = it } } }
        }
    }

    /** 255：首页某分类横排的首屏（IO 调用方自备线程）。 */
    /**
     * 317 · 首页 hero 候选（最近入库，跨库混合）。
     *
     * 【为什么跨库】Rex 的服务器首页 hero 是整服精选，不是某个库的前几个 ——
     * 用户进服务器第一眼是想看"最近有什么新的"，而不是"某个库的第一条"。
     *
     * 336 · 拆成 [heroRaw] + [enrichHeroRatings] 两步：Emby 数据先出首屏，
     * TMDB 评分后台补。原 heroItems 把 16 个 TMDB 请求全 `get()` 完才返回，
     * 首屏 hero 被评分拖住好几秒（net-diag 铁证：Emby 数据早已到，界面仍空位）。
     */
    fun heroRaw(): List<com.kivo.cinema.emby.EmbyHeroItem> {
        // 337：优先用 login 后并发预取的缓存（generation 匹配 = 本次连接），命中即秒返。
        heroPrefetch?.let { if (it.generation == generation) return it.heroes }
        val c = client ?: return emptyList(); val a = auth ?: return emptyList()
        return runCatching { com.kivo.cinema.emby.EmbyHeroSource(c).recent(a, limit = 8) }
            .getOrDefault(emptyList())
    }

    /** 336 · 后台给 hero 补 TMDB 评分（4 并发 + 磁盘缓存命中即快）。查不到留 null ⇒ UI 回退 Emby 分。 */
    fun enrichHeroRatings(context: android.content.Context,
                          raw: List<com.kivo.cinema.emby.EmbyHeroItem>): List<com.kivo.cinema.emby.EmbyHeroItem> {
        if (raw.isEmpty()) return raw
        val pool = java.util.concurrent.Executors.newFixedThreadPool(4)
        return try {
            val futures = raw.map { item ->
                pool.submit(java.util.concurrent.Callable {
                    val rating = runCatching {
                        val tmdb = com.kivo.cinema.tmdb.TmdbClient(context)
                        val id = tmdb.searchId(item.name, item.year, item.isSeries) ?: return@Callable item
                        com.kivo.cinema.tmdb.TmdbDetail(tmdb).details(item.isSeries, id)?.rating
                    }.getOrNull()
                    if (rating != null && rating > 0f) item.copy(tmdbRating = rating.toDouble()) else item
                })
            }
            futures.map { it.get() }
        } finally { pool.shutdown() }
    }

    /** 317 · 继续观看（Emby 原生 Resume：自带位置与总时长）。 */
    fun resumeItems(): List<com.kivo.cinema.emby.EmbyHeroItem> {
        heroPrefetch?.let { if (it.generation == generation) return it.resumed }
        val c = client ?: return emptyList(); val a = auth ?: return emptyList()
        return runCatching { com.kivo.cinema.emby.EmbyHeroSource(c).resume(a, limit = 12) }
            .getOrDefault(emptyList())
    }

    fun rowPage(view: EmbyCatalog.View): List<EmbyItem> {
        val c = client ?: return emptyList(); val a = auth ?: return emptyList()
        return runCatching {
            c.items(a, limit = 12, parentId = view.parentId, types = viewTypes(view), genres = view.genres).items
        }.getOrDefault(emptyList())
    }

    fun openView(view: EmbyCatalog.View) {
        val c = client ?: return; val a = auth ?: return
        drilled = view
        library.open(c, a, view.parentId, viewTypes(view), view.genres)
    }

    fun closeView() { drilled = null }

    private fun viewTypes(view: EmbyCatalog.View) =
        if (view.collectionType == "tvshows") "Series" else "Movie"

    fun back() {
        cancel()
        inLibrary = false
        // Switching servers must not keep authenticated playback or old artwork alive.
        client = null; auth = null; library.clear(); posters.evictAll()
        views = emptyList(); viewsLoaded = false; drilled = null
        server = ""; username = ""; password = ""; savedStarted = false
    }
    /** 254：token 只经 intent 交给播放页的 HttpByteSource 头，不进 URL。 */
    val accessToken: String? get() = auth?.token

    /** 254 D14：详情页收藏/已看走 /Users/{uid}/…，需 userId（播放只需 token）。 */
    val accessUserId: String? get() = auth?.userId

    /** Called on an IO dispatcher; cache keys include host, path and image tag. */
    fun poster(item: EmbyItem): Bitmap? = image(item.posterUrl)

    fun image(url: String): Bitmap? {
        if (url.isBlank()) return null
        posters.get(url)?.let { return it }
        return runCatching {
            val bytes = client?.fetchBytes(url, auth?.token) ?: return null
            BitmapFactory.decodeByteArray(bytes, 0, bytes.size)?.also { posters.put(url, it) }
        }.getOrNull()
    }

    private fun friendlyError(e: Throwable): String = EmbyErrors.reason(e)

    override fun onCleared() { cancel(); library.close(); pool.shutdownNow(); posters.evictAll() }
}
