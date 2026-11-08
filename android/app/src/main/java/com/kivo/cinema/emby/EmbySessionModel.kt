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
import java.net.SocketTimeoutException
import java.net.UnknownHostException
import java.net.ConnectException
import javax.net.ssl.SSLException
import java.util.concurrent.Executors
import java.util.concurrent.Future

/** Memory-only session; never persist passwords or tokens. UI writes run on main. */
class EmbySessionModel : ViewModel() {
    var server by mutableStateOf("")
    var username by mutableStateOf("")
    var password by mutableStateOf("")
    var error by mutableStateOf<String?>(null); private set
    var connecting by mutableStateOf(false); private set
    var inLibrary by mutableStateOf(false); private set
    var items by mutableStateOf(emptyList<EmbyItem>()); private set
    var query by mutableStateOf("")
    var sort by mutableStateOf(0)
    private var client: EmbyClient? = null
    private var auth: EmbyClient.Auth? = null
    private val main = Handler(Looper.getMainLooper())
    private val pool = Executors.newFixedThreadPool(2)
    private var work: Future<*>? = null
    private var generation = 0
    private val posters = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
        override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
    }
    val serverName: String get() = runCatching {
        URI(if (server.contains("://")) server.trim() else "https://${server.trim()}").host
    }.getOrNull() ?: "Emby"

    fun connect() {
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
                    client = c; auth = a; items = list
                    query = ""; sort = 0; inLibrary = true
                }, onFailure = { error = friendlyError(it) })
            }
        }
    }

    fun cancel() { ++generation; work?.cancel(true); connecting = false }
    fun back() {
        cancel()
        inLibrary = false
        // Switching servers must not keep authenticated playback or old artwork alive.
        client = null; auth = null; items = emptyList(); posters.evictAll()
    }
    fun streamUrl(item: EmbyItem): String? = auth?.let { client?.streamUrl(it, item.id) }

    /** Called on an IO dispatcher; cache keys include host, path and image tag. */
    fun poster(item: EmbyItem): Bitmap? {
        if (item.posterUrl.isBlank()) return null
        posters.get(item.posterUrl)?.let { return it }
        return runCatching {
            val bytes = client?.fetchBytes(item.posterUrl) ?: return null
            BitmapFactory.decodeByteArray(bytes, 0, bytes.size)?.also { posters.put(item.posterUrl, it) }
        }.getOrNull()
    }

    private fun friendlyError(e: Throwable): String = when (e) {
        is EmbyClient.HttpFailure -> when (e.code) {
            401 -> "账号或密码未被接受，请检查后重试。"
            403 -> "服务器拒绝了访问，请检查账号权限或服务器的访问限制。"
            404 -> "没有找到 Emby 服务，请检查地址、端口和路径。"
            else -> "服务器暂时无法完成请求（${e.code}），请稍后重试。"
        }
        is UnknownHostException -> "找不到服务器，请检查地址和网络连接。"
        is SocketTimeoutException -> "连接超时，请检查网络或稍后重试。"
        is ConnectException -> "无法连接服务器，请检查端口与服务是否在线。"
        is SSLException -> "无法建立安全连接，请检查服务器证书。"
        is IllegalArgumentException, is java.net.URISyntaxException -> "服务器地址格式不正确，请检查后重试。"
        else -> "连接未完成，请检查网络与服务器地址后重试。"
    }

    override fun onCleared() { cancel(); pool.shutdownNow(); posters.evictAll() }
}
