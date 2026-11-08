package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.IOException
import java.security.MessageDigest

/** Shared image loader. Disk entries retain the response bytes, including PNG alpha. */
object TmdbImageCache {
    private val mem = object : LinkedHashMap<String, Bitmap>(48, .75f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<String, Bitmap>?) = size > 48
    }
    var diskDir: File? = null

    @Synchronized fun get(url: String): Bitmap? = mem[url]
    @Synchronized fun put(url: String, bmp: Bitmap) { mem[url] = bmp }

    private fun diskFile(url: String): File? = diskDir?.let {
        val dir = File(it, "tmdb-images-v2").apply { mkdirs() }
        val key = MessageDigest.getInstance("SHA-256").digest(url.toByteArray())
            .joinToString("") { b -> "%02x".format(b) }
        File(dir, key)
    }

    /** Blocking; callers use Dispatchers.IO. Failed responses are never persisted. */
    fun load(url: String): Bitmap? {
        get(url)?.let { return it }
        val disk = diskFile(url)
        if (disk?.isFile == true) {
            BitmapFactory.decodeFile(disk.absolutePath)?.let { put(url, it); return it }
        }
        // A transient socket reset must not leave a visible poster empty for the whole visit.
        // Retry transport failures once; a missing image or invalid bytes are not transport failures.
        repeat(2) { attempt ->
          try {
            val conn = com.kivo.cinema.tmdb.TmdbClient.staticOpen(url)
            try {
                conn.connectTimeout = 8000
                conn.readTimeout = 20000
                if (conn.responseCode !in 200..299) return null
                val bytes = conn.inputStream.use { it.readBytes() }
                val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                    ?: return null
                put(url, bitmap)
                if (disk != null) runCatching {
                    val temporary = File.createTempFile("image-", ".tmp", disk.parentFile)
                    try { temporary.writeBytes(bytes); temporary.renameTo(disk) }
                    finally { temporary.delete() }
                }
                return bitmap
            } finally { conn.disconnect() }
          } catch (_: IOException) {
              if (attempt == 0) Thread.sleep(200)
          } catch (_: Exception) { return null }
        }
        return null
    }
}

val TmdbPlaceholder = Color(0xFFE9E4EE)

@Composable
fun TmdbImage(
    url: String,
    contentDescription: String,
    modifier: Modifier = Modifier,
    bg: Color = TmdbPlaceholder,
    fit: Boolean = false,
    onLoaded: ((Boolean) -> Unit)? = null,
) {
    var bmp by remember(url) { mutableStateOf(TmdbImageCache.get(url)) }
    val report by rememberUpdatedState(onLoaded)
    LaunchedEffect(url) {
        val loaded = bmp ?: withContext(Dispatchers.IO) { TmdbImageCache.load(url) }
        bmp = loaded
        report?.invoke(loaded != null)
    }
    Box(modifier.background(bg)) {
        bmp?.let {
            Image(it.asImageBitmap(), contentDescription,
                contentScale = if (fit) ContentScale.Fit else ContentScale.Crop,
                modifier = Modifier.fillMaxSize())
        }
    }
}
