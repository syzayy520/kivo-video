package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.sp
import java.net.HttpURLConnection
import java.net.URL

/**
 * TMDB 网络图（237 方案 F2）：URL→Bitmap，内存 LRU + 磁盘缓存（cache/tmdb/），零第三方。
 * 加载中显示底色占位；失败显示 🎬 占位（对应节仍渲染）。
 */
object TmdbImageCache {
    private val mem = LinkedHashMap<String, Bitmap>()
    private const val MAX_MEM = 48
    var diskDir: java.io.File? = null

    @Synchronized
    fun get(url: String): Bitmap? = mem[url]

    @Synchronized
    fun put(url: String, bmp: Bitmap) {
        mem[url] = bmp
        if (mem.size > MAX_MEM) {
            val it = mem.entries.iterator()
            it.next(); it.remove()
        }
    }

    fun diskFile(url: String): java.io.File? {
        val dir = diskDir ?: return null
        dir.mkdirs()
        return java.io.File(dir, Integer.toHexString(url.hashCode()) + ".img")
    }
}

/** 图加载中/失败的占位底色（E12：Rex 缺图为平色块，无 🎬）。 */
val TmdbPlaceholder = Color(0xFFE9E4EE)

@Composable
fun TmdbImage(
    url: String,
    contentDescription: String,
    modifier: Modifier = Modifier,
    bg: Color = TmdbPlaceholder,
    fit: Boolean = false,        // true = ContentScale.Fit（logo 等非满铺图）
    onLoaded: ((Boolean) -> Unit)? = null,   // 加载完成回报（true=成功）——hero 缺图回退标题用
) {
    var bmp by remember(url) { mutableStateOf(TmdbImageCache.get(url)) }
    LaunchedEffect(url) {
        if (bmp != null) return@LaunchedEffect
        val loaded = kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
            val disk = TmdbImageCache.diskFile(url)
            if (disk != null && disk.exists() && disk.length() > 0) {
                BitmapFactory.decodeFile(disk.absolutePath)?.let {
                    TmdbImageCache.put(url, it); return@withContext it
                }
            }
            try {
                val conn = com.kivo.cinema.tmdb.TmdbClient.staticOpen(url)
                conn.connectTimeout = 8000
                conn.readTimeout = 20000
                if (conn.responseCode in 200..299) {
                    val b = BitmapFactory.decodeStream(conn.inputStream)
                    if (b != null) {
                        TmdbImageCache.put(url, b)
                        disk?.let { f ->
                            runCatching {
                                java.io.FileOutputStream(f).use { o ->
                                    b.compress(Bitmap.CompressFormat.JPEG, 82, o)
                                }
                            }
                        }
                    }
                    b
                } else null
            } catch (e: Exception) {
                null
            }
        }
        bmp = loaded
        onLoaded?.invoke(loaded != null)
    }
    Box(modifier.background(bg)) {
        val b = bmp
        if (b != null) {
            Image(b.asImageBitmap(), contentDescription,
                contentScale = if (fit) ContentScale.Fit else ContentScale.Crop,
                modifier = Modifier.fillMaxSize())
        }
    }
}
