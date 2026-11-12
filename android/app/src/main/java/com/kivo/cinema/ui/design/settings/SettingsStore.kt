package com.kivo.cinema.ui.design.settings

import android.content.Context
import android.content.pm.ApplicationInfo
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.home.HomePreferences
import java.io.File

/**
 * 设置页唯一读模型：聚合各真实状态源 + 真实缓存清理动作（261 设计 §三）。
 * autoplay 仍由 HomePreferences 持久化（不另起 prefs，避免双源）；其余为只读真实值。
 * 无对应后端的能力不在此伪造（模块/服务/PRO/iCloud 等不纳入）。
 */
internal class SettingsStore(context: Context) {
    private val app = context.applicationContext
    private val home = HomePreferences(app)
    private val tmdb = TmdbClient(app)

    var previewAutoplay: Boolean
        get() = home.autoplay
        set(value) { home.autoplay = value }

    val clearTextOk: Boolean
        get() = app.applicationInfo.flags and ApplicationInfo.FLAG_USES_CLEARTEXT_TRAFFIC != 0

    val tmdbProxy: String? get() = tmdb.proxyAddr()
    val tmdbKeyConfigured: Boolean get() = tmdb.apiKey().isNotEmpty()

    val versionLabel: String
        get() = runCatching {
            val pi = app.packageManager.getPackageInfo(app.packageName, 0)
            @Suppress("DEPRECATION")
            "${pi.versionName} (${pi.versionCode})"
        }.getOrDefault("未知")

    // 真实缓存目标：图片磁盘缓存 cacheDir/tmdb-images-v2；评分接口缓存 filesDir/tmdb_cache.json。
    private val imageDir get() = File(app.cacheDir, "tmdb-images-v2")
    private val ratingFile get() = File(app.filesDir, "tmdb_cache.json")

    fun imageCacheBytes(): Long = dirBytes(imageDir)
    fun ratingCacheBytes(): Long = if (ratingFile.isFile) ratingFile.length() else 0L

    /** 真实清理：删图片磁盘缓存 + 清进程内图缓存 + 删评分缓存。返回释放字节数。 */
    fun clearCaches(): Long {
        val freed = imageCacheBytes() + ratingCacheBytes()
        runCatching { imageDir.listFiles()?.forEach { it.delete() } }
        TmdbImageCache.clearMemory()
        runCatching { ratingFile.delete() }
        return freed
    }

    private fun dirBytes(dir: File): Long =
        dir.listFiles()?.sumOf { if (it.isFile) it.length() else 0L } ?: 0L
}

internal fun formatBytes(bytes: Long): String = when {
    bytes >= 1L shl 30 -> "%.2f GB".format(bytes / 1073741824.0)
    bytes >= 1L shl 20 -> "%.1f MB".format(bytes / 1048576.0)
    bytes >= 1L shl 10 -> "%.0f KB".format(bytes / 1024.0)
    else -> "$bytes B"
}