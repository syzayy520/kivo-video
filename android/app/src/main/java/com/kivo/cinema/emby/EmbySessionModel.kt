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
    private val pool = Executors.newFixedThreadPool(2)
    private var work: Future<*>? = null
    private var generation = 0
    private val posters = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
        override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
    }
    private var savedName: String? = null
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
                val a = c.login(user, secret)
                Triple(c, a, c.items(a))
            }
            main.post {
                if (ticket != generation) return@post
                connecting = false
                result.fold(onSuccess = { (c, a, list) ->
                    client = c; auth = a; library.attach(c, a, list)
                    inLibrary = true
                    loadViews(c, a)
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
