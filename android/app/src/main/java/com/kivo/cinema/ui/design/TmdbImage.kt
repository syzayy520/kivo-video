package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Protocol
import okhttp3.Request
import java.io.File
import java.io.IOException
import java.security.MessageDigest
import java.util.concurrent.TimeUnit

/** Shared image loader. Disk entries retain the response bytes, including PNG alpha. */
object TmdbImageCache {
    private val mem = object : LinkedHashMap<String, Bitmap>(48, .75f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<String, Bitmap>?) = size > 48
    }
    var diskDir: File? = null

    @Synchronized fun get(url: String): Bitmap? = mem[url]
    @Synchronized fun put(url: String, bmp: Bitmap) { mem[url] = bmp }
    @Synchronized fun clearMemory() { mem.clear() }

    private fun diskFile(url: String): File? = diskDir?.let {
        val dir = File(it, "tmdb-images-v2").apply { mkdirs() }
        val key = MessageDigest.getInstance("SHA-256").digest(url.toByteArray())
            .joinToString("") { b -> "%02x".format(b) }
        File(dir, key)
    }

    /** 图片专用：现代 TLS + HTTP/2，ClientHello 与浏览器一致（绕过按旧指纹的 reset）。 */
    private val http: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(8, TimeUnit.SECONDS)
            .readTimeout(20, TimeUnit.SECONDS)
            .protocols(listOf(Protocol.HTTP_2, Protocol.HTTP_1_1))
            .build()
    }

    /** Blocking; callers use Dispatchers.IO. Failed responses are never persisted. */
    fun load(url: String, token: String? = null): Bitmap? {
        // 缓存键不含 token：同一张图不因换 token 重复下载（URL 已含 itemId+tag）。
        get(url)?.let { return it }
        val disk = diskFile(url)
        if (disk?.isFile == true) {
            decodeImageFileCapped(disk.absolutePath)?.let {
                put(url, it)
                // D369：只给**带 token 的图**（Emby 海报/缩略图）记一行，量冷/热用，不淹没日志。
                if (token != null) com.kivo.cinema.net.NetDiag.log("[img] disk-hit ${url.takeLast(28)}")
                return it
            }
        }
        // OkHttp：现代 TLS + HTTP/2，ClientHello 与浏览器一致，避免被中间设备按旧指纹 reset。
        // 重试一次传输失败；404/坏字节不重试。
        repeat(2) { attempt ->
            try {
                val req = Request.Builder().url(url).apply {
                    if (!token.isNullOrBlank()) header("X-Emby-Token", token)
                }.build()
                http.newCall(req).execute().use { resp ->
                    if (!resp.isSuccessful) return null
                    val bytes = resp.body?.bytes() ?: return null
                    val bitmap = decodeImageCapped(bytes) ?: return null
                    put(url, bitmap)
                    if (token != null) com.kivo.cinema.net.NetDiag.log("[img] net ${url.takeLast(28)}")
                    if (disk != null) runCatching {
                        val temporary = File.createTempFile("image-", ".tmp", disk.parentFile)
                        try { temporary.writeBytes(bytes); temporary.renameTo(disk) }
                        finally { temporary.delete() }
                    }
                    return bitmap
                }
            } catch (_: IOException) {
                if (attempt == 0) Thread.sleep(200)
            } catch (_: Exception) { return null }
        }
        return null
    }
}

/** 255 #30：解码上限——original 海报可达 2000×3000+，按最长边 ≤1600 采样，防 48 格缓存撑爆内存。 */
internal fun decodeImageCapped(bytes: ByteArray, maxDim: Int = 1600): Bitmap? {
    val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
    BitmapFactory.decodeByteArray(bytes, 0, bytes.size, bounds)
    val opts = BitmapFactory.Options().apply {
        inSampleSize = cappedSample(bounds.outWidth, bounds.outHeight, maxDim)
    }
    return BitmapFactory.decodeByteArray(bytes, 0, bytes.size, opts)
}

internal fun decodeImageFileCapped(path: String, maxDim: Int = 1600): Bitmap? {
    val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
    BitmapFactory.decodeFile(path, bounds)
    val opts = BitmapFactory.Options().apply {
        inSampleSize = cappedSample(bounds.outWidth, bounds.outHeight, maxDim)
    }
    return BitmapFactory.decodeFile(path, opts)
}

private fun cappedSample(w: Int, h: Int, maxDim: Int): Int {
    if (w <= 0 || h <= 0) return 1
    var sample = 1
    val max = maxOf(w, h)
    while (max / sample > maxDim) sample *= 2
    return sample
}

@Composable
fun TmdbImage(
    url: String,
    contentDescription: String,
    modifier: Modifier = Modifier,
    bg: Color = DesignTokens.posterPlaceholder,
    fit: Boolean = false,
    onLoaded: ((Boolean) -> Unit)? = null,
    loader: (suspend (String) -> Bitmap?)? = null,
) {
    var bmp by remember(url) { mutableStateOf(TmdbImageCache.get(url)) }
    val report by rememberUpdatedState(onLoaded)
    val fetch by rememberUpdatedState(loader)
    LaunchedEffect(url) {
        val loaded = bmp ?: withContext(Dispatchers.IO) {
            fetch?.invoke(url)?.also { TmdbImageCache.put(url, it) } ?: TmdbImageCache.load(url)
        }
        bmp = loaded
        report?.invoke(loaded != null)
    }
    Box(modifier) {
        // 首帧/加载失败/unsupported 时显示规范占位；bg=Transparent 时组件只留 glyph，
        // 不画不透明底（PlatformCards/StudioCards 靠此规避浅底显露坑）。
        PosterPlaceholder(Modifier.fillMaxSize(), bg = bg)
        bmp?.let {
            Image(it.asImageBitmap(), contentDescription,
                contentScale = if (fit) ContentScale.Fit else ContentScale.Crop,
                modifier = Modifier.fillMaxSize())
        }
    }
}
