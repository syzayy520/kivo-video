package com.kivo.cinema.library

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.util.concurrent.ConcurrentHashMap

/**
 * NAS 片源 → TMDB id 的缓存。
 *
 * ## 为什么需要
 * 详情页的「收藏 / 已看」是按 **TMDB id** 记的（[com.kivo.cinema.media.marks.FilmKey]）。
 * 本地片源没有 id，所以一直只能给"禁用态"——用户点不了收藏。
 *
 * 而详情页的 TMDB 增强（[com.kivo.cinema.ui.detail.SmbDetail]）本来就会搜到 id，
 * 只是用完就丢了。把它存下来后：**看过一次，下次进来收藏就能用**。
 *
 * 首次进入仍是禁用态（那一刻还不知道 id），这是诚实的上限，不假装可用。
 */
object NasTmdbIdStore {

    private val mem = ConcurrentHashMap<String, Int>()
    @Volatile private var loaded = false
    private var disk: File? = null

    private fun key(title: String, year: Int?) = "${title.trim()}|${year ?: 0}"

    /** 同步查：有就返回 id，没有返回 null（调用方据此决定收藏按钮可用性）。 */
    fun get(context: Context, title: String, year: Int?): Int? {
        if (title.isBlank()) return null
        ensureLoaded(context)
        return mem[key(title, year)]
    }

    /** 记录一次解析结果（同标题同一年份只写一次）。 */
    fun put(context: Context, title: String, year: Int?, id: Int) {
        if (title.isBlank() || id <= 0) return
        ensureLoaded(context)
        val k = key(title, year)
        if (mem[k] == id) return
        mem[k] = id
        save(context)
    }

    private fun ensureLoaded(context: Context) {
        if (loaded) return
        synchronized(this) {
            if (loaded) return
            loaded = true
            val f = File(File(context.filesDir, "nas"), "nas-tmdb-ids.json")
            disk = f
            runCatching {
                if (!f.exists()) return@runCatching
                val o = JSONObject(f.readText())
                o.keys().forEach { k -> o.optInt(k, 0).takeIf { it > 0 }?.let { mem[k] = it } }
            }
        }
    }

    private fun save(context: Context) {
        synchronized(this) {
            runCatching {
                val f = disk ?: File(File(context.filesDir, "nas"), "nas-tmdb-ids.json").also { disk = it }
                f.parentFile?.mkdirs()
                val o = JSONObject()
                mem.entries.take(MAX_ENTRIES).forEach { (k, v) -> o.put(k, v) }
                f.writeText(o.toString())
            }
        }
    }

    private const val MAX_ENTRIES = 2000
}
