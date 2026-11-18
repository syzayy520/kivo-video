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
import com.kivo.cinema.emby.session.EmbyServerSession
import com.kivo.cinema.emby.session.EmbySessionRegistry
import java.net.URI
import java.util.concurrent.Callable
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
    // 337：login 后并发预取的首屏数据。338：owner=sourceId（不再用 generation），
    // 这样切服后台连接期间 ++generation 不会让旧页 hero 变空；只在原子提交时换 owner。
    @Volatile private var heroPrefetch: HeroPrefetch? = null
    private class HeroPrefetch(val owner: String, val heroes: List<EmbyHeroItem>,
                               val resumed: List<EmbyHeroItem>)
    private var work: Future<*>? = null
    private var generation = 0
    private val sessions = EmbySessionRegistry()  // 338：按服缓存已连接会话，避免回资源库重走
    private var activeId by mutableStateOf<String?>(null)  // state：UI 靠它感知原地切服
    private var pendingId: String? = null
    /** 338：当前页面数据真正所属的服（顶栏高亮/详情 intent 都用它，避免 token 与服错配）。 */
    val currentSourceId: String? get() = activeId
    private val posters = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
        override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
    }
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
                    savedName = source.name; activeId = id
                    connect()
                }, { error = if (it is com.kivo.cinema.sources.SourceFailure) it.reason else "无法读取连接，请返回资源库后重试" })
            }
        }
    }

    val serverName: String get() = savedName ?: runCatching {
        URI(if (server.contains("://")) server.trim() else "https://${server.trim()}").host
    }.getOrNull() ?: "Emby"

    /** 338 · 后台纯取数：登录 + 并行 hero/resume + items。不碰任何 UI state，首次连接与切服共用。 */
    private class Connected(val c: EmbyClient, val a: EmbyClient.Auth,
                            val list: EmbyClient.Page, val heroes: List<EmbyHeroItem>,
                            val resumed: List<EmbyHeroItem>)

    private fun connectCore(address: String, user: String, secret: String): Connected {
        // D367：会话复用（真机 login 一次 1221ms，不能每次进浏览页都重付）。
        val t0 = System.currentTimeMillis()
        val (c, a) = EmbyAuthCache.connect(address, user, secret)
        val t1 = System.currentTimeMillis()
        com.kivo.cinema.net.NetDiag.log("[emby-connect] login ${t1 - t0}ms")
        // login 拿到 token 后立即并发预取 hero/resume：与 items 重叠，首屏空白期大幅缩短。
        val recentFuture = pool.submit(Callable<List<EmbyHeroItem>> {
            runCatching { EmbyHeroSource(c).recent(a, limit = 8) }.getOrDefault(emptyList())
        })
        val resumeFuture = pool.submit(Callable<List<EmbyHeroItem>> {
            runCatching { EmbyHeroSource(c).resume(a, limit = 12) }.getOrDefault(emptyList())
        })
        val list = c.items(a)
        val t2 = System.currentTimeMillis()
        com.kivo.cinema.net.NetDiag.log("[emby-connect] items ${t2 - t1}ms total ${t2 - t0}ms count=${list.items.size}")
        val hs = runCatching { recentFuture.get() }.getOrDefault(emptyList())
        val rs = runCatching { resumeFuture.get() }.getOrDefault(emptyList())
        return Connected(c, a, list, hs, rs)
    }

    /** 一次性把连接结果提交到 UI（首次连接、切服成功共用）。调用前 activeId 须已指向目标服。 */
    private fun submitConnected(conn: Connected, ticket: Int) {
        client = conn.c; auth = conn.a
        library.attach(conn.c, conn.a, conn.list)
        heroPrefetch = HeroPrefetch(activeId ?: "", conn.heroes, conn.resumed)
        inLibrary = true
        views = emptyList(); viewsLoaded = false   // 切服时不能把旧服分类留给新服
        loadViews(conn.c, conn.a)
        pool.submit {
            val pretty = runCatching { conn.c.publicServerName() }.getOrNull()
            if (pretty == null) com.kivo.cinema.net.NetDiag.log("[emby] ServerName 查询失败，保留源名 $savedName")
            else main.post { if (ticket == generation) savedName = pretty }
        }
    }

    private fun connect() {
        if (connecting || server.isBlank() || username.isBlank()) return
        error = null; connecting = true
        val ticket = ++generation
        val address = server.trim(); val user = username.trim(); val secret = password
        work = pool.submit {
            val result = runCatching { connectCore(address, user, secret) }
            main.post {
                if (ticket != generation) return@post
                connecting = false
                result.fold(onSuccess = { submitConnected(it, ticket) },
                            onFailure = { error = friendlyError(it) })
            }
        }
    }

    /**
     * 338 · 原地切服：不重启 Activity，不回资源库。
     * · 目标服连过 ⇒ 原样恢复（列表滚动/搜索/评分全在），零 loading；
     * · 未连过 ⇒ 旧页保持不动，后台取数，成功后一次性原子替换；失败留日志并保留当前服。
     */
    fun switchTo(context: android.content.Context, id: String) {
        if (id == activeId || id == pendingId) return
        val source = runCatching {
            com.kivo.cinema.sources.SourceRepository(
                com.kivo.cinema.sources.storage.SourceStore(context.applicationContext))
                .read().getOrThrow().firstOrNull { it.id == id }
        }.getOrNull()
        if (source == null || !com.kivo.cinema.sources.access.SourceEligibility.visible(source,
                com.kivo.cinema.sources.access.SourceAccessSession.unlocked)) {
            com.kivo.cinema.net.NetDiag.log("[emby-switch] $id 不可访问，忽略切服")
            return
        }
        if (activeId != null && inLibrary) saveCurrent()
        pendingId = id
        sessions.find(id)?.let { cached ->
            activeId = id; pendingId = null
            restore(cached)
            com.kivo.cinema.net.NetDiag.log("[emby-switch] $id 命中缓存，秒切")
            return
        }
        com.kivo.cinema.net.NetDiag.log("[emby-switch] $id 未缓存，后台连接（旧页保留）")
        val ticket = ++generation
        val address = source.address.trim(); val user = source.username.trim(); val secret = source.password
        work = pool.submit {
            val result = runCatching { connectCore(address, user, secret) }
            main.post {
                if (ticket != generation) return@post
                pendingId = null
                result.fold(onSuccess = { conn ->
                    activeId = id; savedName = source.name
                    submitConnected(conn, ticket)
                    com.kivo.cinema.net.NetDiag.log("[emby-switch] $id 原子切换完成")
                }, onFailure = { com.kivo.cinema.net.NetDiag.log("[emby-switch] $id 失败：${friendlyError(it)}，保留当前服") })
            }
        }
    }

    private fun saveCurrent() {
        val c = client ?: return; val a = auth ?: return
        val id = activeId ?: return
        sessions.save(id, EmbyServerSession(c, a, library.snapshot(), views,
            heroPrefetch?.heroes.orEmpty(), heroPrefetch?.resumed.orEmpty(), savedName ?: serverName))
    }

    private fun restore(s: EmbyServerSession) {
        client = s.client; auth = s.auth
        library.restore(s.list, s.client, s.auth)
        views = s.views; viewsLoaded = true; drilled = null
        heroPrefetch = HeroPrefetch(activeId ?: "", s.heroes, s.resumed)
        savedName = s.savedName
        server = ""; username = ""; password = ""   // 凭据不缓存；详情/播放只用 auth 里的 token。
        error = null; inLibrary = true
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

    /** 317/336 · hero 数据先出首屏（Emby 直连），TMDB 评分后台补。 */
    fun heroRaw(): List<com.kivo.cinema.emby.EmbyHeroItem> {
        heroPrefetch?.let { if (it.owner == activeId) return it.heroes }
        val c = client ?: return emptyList(); val a = auth ?: return emptyList()
        return runCatching { com.kivo.cinema.emby.EmbyHeroSource(c).recent(a, limit = 8) }
            .getOrDefault(emptyList())
    }

    /** 336 · 后台给 hero 补 TMDB 评分（4 并发 + 磁盘缓存命中即快）。 */
    fun enrichHeroRatings(context: android.content.Context,
                          raw: List<com.kivo.cinema.emby.EmbyHeroItem>): List<com.kivo.cinema.emby.EmbyHeroItem> {
        if (raw.isEmpty()) return raw
        val pool = java.util.concurrent.Executors.newFixedThreadPool(4)
        return try {
            val futures = raw.map { item ->
                pool.submit(Callable {
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
        heroPrefetch?.let { if (it.owner == activeId) return it.resumed }
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
        inLibrary = false; pendingId = null
        // Switching servers must not keep authenticated playback or old artwork alive.
        client = null; auth = null; library.clear(); posters.evictAll(); sessions.clear()
        views = emptyList(); viewsLoaded = false; drilled = null
        server = ""; username = ""; password = ""; savedStarted = false; activeId = null
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

    override fun onCleared() { cancel(); library.close(); pool.shutdownNow(); posters.evictAll(); sessions.clear() }
}
