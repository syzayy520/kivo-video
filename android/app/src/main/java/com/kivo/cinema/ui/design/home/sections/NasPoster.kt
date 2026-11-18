package com.kivo.cinema.ui.design.home.sections

import android.content.Context
import android.graphics.Bitmap
import android.util.LruCache
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.SmbContinueKey
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbCredential
import com.kivo.cinema.smb.SmbReader
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.ui.design.decodeImageCapped
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.withPermit
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream

/** 同时最多处理 3 张封面：12 张卡 × 6 路候选会瞬间打 70+ 次 TMDB 搜索，容易被限流。 */
private val posterPermits = kotlinx.coroutines.sync.Semaphore(3)

/** 首页 NAS 封面缓存：跨卡片/跨滚动复用，按字节限。 */
private val nasPosterCache = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
    override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
}

/**
 * NAS 影片封面：**库内同目录图片 → TMDB 兜底 → 渐变占位**（用户要求的三级链）。
 *
 * 只负责"把一张封面取来并画出来"：读 SMB / 调 TMDB 都在 IO 线程，失败静默落到占位，
 * 不抛异常、不白块。缓存以 [HomeEntry.key] 为键，滚动回来不重复读。
 */
@Composable
internal fun NasPoster(
    entry: HomeEntry,
    modifier: Modifier = Modifier,
    shape: Shape = RoundedCornerShape(12.dp),
) {
    val context = LocalContext.current
    var bitmap by remember(entry.key) { mutableStateOf(nasPosterCache.get(entry.key)) }
    LaunchedEffect(entry.key) {
        if (bitmap != null) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) {
            posterPermits.withPermit {
                resolvePoster(context, entry)?.also { nasPosterCache.put(entry.key, it) }
            }
        }
    }
    val bmp = bitmap
    if (bmp == null) {
        NasPosterPlaceholder(entry, modifier, shape)
    } else {
        Box(modifier.fillMaxSize().clip(shape).background(Color(0xFFE8E6EF))) {
            Image(
                bmp.asImageBitmap(), contentDescription = null,
                contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize(),
            )
        }
    }
}

/**
 * 取 NAS 条目海报（库内同目录图 → TMDB → 空）。
 *
 * 详情页也要用同一套（否则首页有图、点进详情反而空），故不再 private。
 * 阻塞（SMB 读图/网络），必须在 IO 线程调用。
 */
internal fun resolvePoster(context: Context, entry: HomeEntry): Bitmap? {
    NetDiag.log("nas-poster start ${entry.title} y=${entry.year} smb=${entry.smbPosterPath}")
    // ⓪ 磁盘缓存：TMDB 图片 CDN 在国内不稳，成功过一次就永久可用，避免下次又落占位
    val disk = diskCacheFile(context, entry.key)
    if (disk.length() > 0L) {
        runCatching { decodeImageCapped(disk.readBytes()) }.getOrNull()?.let {
            NetDiag.log("nas-poster disk-hit ${entry.title}"); return it
        }
    }
    // ① 库里同目录图片（真机实测 7% 的影片有，CMCT 是 `片名.年份.jpg`、CHD 是 `cover.jpg`）
    entry.smbPosterPath?.let { path ->
        readSmbImage(context, entry.key, path)?.let {
            saveDisk(disk, it); NetDiag.log("nas-poster smb-ok ${entry.title}"); return it
        }
    }
    // ② TMDB 兜底：先当电影搜，查不到再当剧集搜（NAS 分类不一定准，两种都试一次）
    val tmdb = TmdbClient(context.applicationContext)
    // 候选顺序：完整名 → 纯中文段 → 英文名，各试电影/剧集（剧集目录名常是"中文 英文 S01E01"混排）
    val cn = chineseOnly(entry.title)
    val url = buildList {
        add(Triple(entry.title, entry.year, false))
        cn?.let { add(Triple(it, entry.year, false)) }
        add(Triple(entry.searchAlias, entry.year, false))
        add(Triple(entry.title, entry.year, true))
        cn?.let { add(Triple(it, entry.year, true)) }
        add(Triple(entry.searchAlias, entry.year, true))
    }.firstNotNullOfOrNull { (q, y, tv) -> q?.let { tmdb.posterUrlFor(it, y, tv) } }
    NetDiag.log("nas-poster tmdb ${entry.title} url=$url")
    if (url == null) return null
    // ③ 网络图也失败 → null，外层落渐变占位
    return fetchRemoteImage(tmdb, url)?.also {
        saveDisk(disk, it)
        NetDiag.log("nas-poster done ${entry.title} bmp=${it.width}x${it.height}")
    }.also { if (it == null) NetDiag.log("nas-poster fail ${entry.title}") }
}

/** 从混排名里抠出中文段（`69两头勾 6ixtynin9 The Series` → `69两头勾`）；与原名相同则视为无。 */
private fun chineseOnly(s: String): String? {
    val m = Regex("""[A-Za-z0-9]*[一-龥][一-龥A-Za-z0-9]*""").find(s) ?: return null
    return m.value.trim().takeIf { it.length >= 2 && it != s }
}

private fun diskCacheFile(context: Context, key: String): File =
    File(File(context.cacheDir, "nas-poster"), md5(key) + ".jpg")

private fun saveDisk(target: File, bitmap: Bitmap) {
    runCatching {
        target.parentFile?.mkdirs()
        FileOutputStream(target).use { bitmap.compress(Bitmap.CompressFormat.JPEG, 90, it) }
    }
}

private fun md5(s: String): String =
    java.security.MessageDigest.getInstance("MD5").digest(s.toByteArray())
        .joinToString("") { "%02x".format(it) }

private fun readSmbImage(context: Context, key: String, posterPath: String): Bitmap? {
    val parts = SmbContinueKey.parse(key) ?: return null
    val record = runCatching {
        SourceStore(context).read().getOrNull()?.firstOrNull { it.id == parts.sourceId }
    }.getOrNull() ?: return null
    val address = runCatching { SmbAddress.parse(record.address) }.getOrNull() ?: return null
    val credential = SmbCredential(username = record.username, password = record.password)
    return runCatching {
        SmbReader(SmbAddress(address.host, address.port, parts.share), credential, posterPath).use { reader ->
            reader.open()
            val out = ByteArrayOutputStream()
            val buf = ByteArray(64 * 1024)
            var offset = 0L
            while (out.size() < MAX_POSTER_BYTES) {
                val n = reader.pread(offset, buf, 0, buf.size)
                if (n <= 0) break
                out.write(buf, 0, n)
                offset += n
            }
            decodeImageCapped(out.toByteArray())
        }
    }.getOrNull()
}

private fun fetchRemoteImage(tmdb: TmdbClient, url: String): Bitmap? {
    // 图片 CDN 偶发慢/超时：放宽超时并重试，失败原因写进日志便于诊断（不许静默）。
    var last: Throwable? = null
    repeat(3) {
        try {
            val conn = tmdb.openConnection(url)
            conn.connectTimeout = 8000
            conn.readTimeout = 8000
            val bytes = conn.inputStream.use { it.readBytes() }
            return decodeImageCapped(bytes)
        } catch (e: Throwable) {
            last = e
        }
    }
    NetDiag.log("nas-poster net-fail ${last?.javaClass?.simpleName}: ${last?.message}")
    return null
}

private const val MAX_POSTER_BYTES = 4 * 1024 * 1024
