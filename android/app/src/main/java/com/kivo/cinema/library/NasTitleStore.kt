package com.kivo.cinema.library

import android.content.Context
import com.kivo.cinema.tmdb.TmdbClient
import org.json.JSONObject
import java.io.File
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Semaphore

/**
 * NAS 片名的**本地化缓存**：文件名解析出的英文名 → TMDB 的当前语言标题。
 *
 * ## 为什么需要它
 * 片库索引只存文件名，卡片标题由 [MediaTitleParser] 现解析。文件名是英文（`Cold.Case.Gleen...`）
 * 时卡片就是英文 —— 与系统语言无关，用户看到的就是"中文系统里一堆英文片名"。
 * 这里把 TMDB 的译文按「原名|年份」缓存下来，卡片渲染时同步查、查不到再异步补。
 *
 * ## 三条硬约束
 * 1. **不阻塞渲染**：[peek] 只读内存/磁盘，绝不发网络；网络只在 [resolve] 里，由调用方放到 IO 线程。
 * 2. **必须限流**：首页一次可见几十张卡，并发搜索会打爆 TMDB 配额（实测 aq 429）。
 * 3. **认不出就返回 null**：让上层继续显示文件名，不能把标题变空。
 */
object NasTitleStore {

    private val mem = ConcurrentHashMap<String, String>()
    private val gate = Semaphore(3)
    @Volatile private var diskLoaded = false
    private var disk: File? = null

    private fun key(title: String, year: Int?) = "${title.trim()}|${year ?: 0}"

    /** 同步查（内存 → 磁盘）。没有就返回 null，调用方继续用文件名。 */
    fun peek(context: Context, title: String, year: Int?): String? {
        ensureLoaded(context)
        return mem[key(title, year)]
    }

    /**
     * 解析并缓存。**阻塞**（网络），调用方必须在 IO 线程调用。
     *
     * 先按电影搜，搜不到再按剧集搜 —— 片库分类目录名不一定准（`剧集/` 下也可能放电影）。
     */
    fun resolve(context: Context, title: String, year: Int?): String? {
        if (title.isBlank()) return null
        ensureLoaded(context)
        peek(context, title, year)?.let { return it }

        gate.acquire()
        try {
            // 进来后再查一次：并发场景下可能已被别的卡片解析完。
            mem[key(title, year)]?.let { return it }
            val client = TmdbClient(context.applicationContext)
            if (client.apiKey().isEmpty()) return null
            val hit = client.localizedTitle(title, year, series = false)
                ?: client.localizedTitle(title, year, series = true)
                ?: return null
            mem[key(title, year)] = hit
            save(context)
            return hit
        } catch (_: InterruptedException) {
            return null
        } finally {
            gate.release()
        }
    }

    private fun ensureLoaded(context: Context) {
        if (diskLoaded) return
        synchronized(this) {
            if (diskLoaded) return
            diskLoaded = true
            val f = File(File(context.filesDir, "nas"), "nas-titles.json")
            disk = f
            runCatching {
                if (!f.exists()) return@runCatching
                val o = JSONObject(f.readText())
                o.keys().forEach { k -> o.optString(k).takeIf { it.isNotBlank() }?.let { mem[k] = it } }
            }
        }
    }

    private fun save(context: Context) {
        synchronized(this) {
            runCatching {
                val f = disk ?: File(File(context.filesDir, "nas"), "nas-titles.json").also { disk = it }
                f.parentFile?.mkdirs()
                val o = JSONObject()
                // 上限保护：缓存是"锦上添花"，别让它无限膨胀拖慢启动。
                mem.entries.take(MAX_ENTRIES).forEach { (k, v) -> o.put(k, v) }
                f.writeText(o.toString())
            }
        }
    }

    /** 2000 条 ≈ 一个中型片库的九成条目；超出不再增长（先到先得）。 */
    private const val MAX_ENTRIES = 2000
}
