package com.kivo.cinema.stats

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * 观看数据域（237 方案 F1，零第三方）：
 * ① 续播进度：mediaKey(path) → {positionMs, durationMs, updatedAt}
 * ② 观看统计：按天累计观看毫秒数（统计页柱状图数据源）
 *
 * 持久化 = filesDir/watch.json（同 PrivateStore 模式）。单线程调用方自备同步。
 */
class WatchStore(context: Context) {

    data class Progress(val positionMs: Long, val durationMs: Long, val updatedAtMs: Long)

    private val file: File = File(context.filesDir, "watch.json")
    private val progress = LinkedHashMap<String, Progress>()   // key=path
    private val dailyMs = LinkedHashMap<String, Long>()        // key=yyyy-MM-dd → 观看 ms
    private var loaded = false

    @Synchronized
    private fun load() {
        if (loaded) return
        loaded = true
        runCatching {
            if (!file.exists()) return
            val root = JSONObject(file.readText())
            root.optJSONObject("progress")?.let { p ->
                p.keys().forEach { k ->
                    val o = p.getJSONObject(k)
                    progress[k] = Progress(o.getLong("pos"), o.getLong("dur"), o.getLong("at"))
                }
            }
            root.optJSONObject("daily")?.let { d ->
                d.keys().forEach { k -> dailyMs[k] = d.getLong(k) }
            }
        }
    }

    @Synchronized
    private fun persist() {
        val p = JSONObject()
        progress.forEach { (k, v) ->
            p.put(k, JSONObject().put("pos", v.positionMs)
                .put("dur", v.durationMs).put("at", v.updatedAtMs))
        }
        val d = JSONObject()
        dailyMs.forEach { (k, v) -> d.put(k, v) }
        file.writeText(JSONObject().put("progress", p).put("daily", d).toString())
    }

    /** 播放器上报：位置 + 时长 + 本次新增观看毫秒。 */
    @Synchronized
    fun report(key: String, positionMs: Long, durationMs: Long, watchedDeltaMs: Long) {
        load()
        if (positionMs > 0) {
            progress[key] = Progress(positionMs, durationMs, System.currentTimeMillis())
        } else {
            progress.remove(key)   // 看完/重置：清除续播位
        }
        if (watchedDeltaMs > 0) {
            val day = java.text.SimpleDateFormat("yyyy-MM-dd",
                java.util.Locale.US).format(java.util.Date())
            dailyMs[day] = (dailyMs[day] ?: 0L) + watchedDeltaMs
        }
        persist()
    }

    /** 续播位（无记录/接近片尾 95% → null）。 */
    @Synchronized
    fun progressOf(key: String): Progress? {
        load()
        val p = progress[key] ?: return null
        if (p.durationMs > 0 && p.positionMs * 100 / p.durationMs >= 95) return null
        return p
    }

    /** 最近看过（统计页/继续观看列表用，按 updatedAt 倒序）。 */
    @Synchronized
    fun recentKeys(limit: Int = 10): List<String> {
        load()
        return progress.entries.sortedByDescending { it.value.updatedAtMs }
            .take(limit).map { it.key }
    }

    /** 最近 n 天每日观看毫秒（今天在末位；缺日补零）。 */
    @Synchronized
    fun dailyMs(days: Int): List<Pair<String, Long>> {
        load()
        val fmt = java.text.SimpleDateFormat("yyyy-MM-dd", java.util.Locale.US)
        val out = mutableListOf<Pair<String, Long>>()
        for (i in days - 1 downTo 0) {
            val day = fmt.format(java.util.Date(System.currentTimeMillis() - i * 86_400_000L))
            out.add(day to (dailyMs[day] ?: 0L))
        }
        return out
    }
}
