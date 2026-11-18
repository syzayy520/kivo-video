package com.kivo.cinema.stats

import android.content.Context
import android.util.AtomicFile
import com.kivo.cinema.net.NetDiag
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * 观看数据域（237 方案 F1，零第三方）：
 * ① 续播进度：mediaKey(path) → {positionMs, durationMs, updatedAt}
 * ② 观看统计：按天累计观看毫秒数（统计页柱状图数据源）
 *
 * 持久化 = filesDir/watch.json（同 PrivateStore 模式）。单线程调用方自备同步。
 *
 * @param fileName 文件名；默认 `watch.json`。给测试传另一个名字，
 *   避免 instrumentation 动到用户真实的观看进度。
 */
class WatchStore(context: Context, fileName: String = "watch.json") {

    data class Progress(val positionMs: Long, val durationMs: Long, val updatedAtMs: Long)

    private val file: File = File(context.filesDir, fileName)
    // 307：原实现用 `file.writeText()` 直接覆写 —— 写到一半掉电/崩溃就留下半个 JSON，
    // 下次 `load()` 解析失败（且异常被 runCatching 吞掉）⇒ **观看进度与统计静默全丢**。
    // 改用 AtomicFile（与 [com.kivo.cinema.sources.storage.SourceStore] 同一写法）：
    // 先写 `.new`、fsync 后再原子改名，任何时刻磁盘上都有一份完整文件。
    private val disk = AtomicFile(file)
    private val progress = LinkedHashMap<String, Progress>()   // key=path
    private val dailyMs = LinkedHashMap<String, Long>()        // key=yyyy-MM-dd → 观看 ms
    private var loaded = false

    @Synchronized
    private fun load() {
        if (loaded) return
        loaded = true
        if (!file.exists() && !File(file.path + ".bak").exists()) return
        val text = try {
            disk.readFully().toString(Charsets.UTF_8)
        } catch (e: Exception) {
            // 读失败（含 `AtomicFile` 自己回滚到 `.bak` 后仍失败）：要说出来，不许静默。
            NetDiag.log("[watch] 读 watch.json 失败：${e.javaClass.simpleName}: ${e.message}")
            return
        }
        try {
            val root = JSONObject(text)
            root.optJSONObject("progress")?.let { p ->
                p.keys().forEach { k ->
                    val o = p.getJSONObject(k)
                    progress[k] = Progress(o.getLong("pos"), o.getLong("dur"), o.getLong("at"))
                }
            }
            root.optJSONObject("daily")?.let { d ->
                d.keys().forEach { k -> dailyMs[k] = d.getLong(k) }
            }
        } catch (e: Exception) {
            // 307：损坏时必须留证据。watch.json 是**每秒级高频写**的文件，
            // 不能像源列表那样"永不覆盖"（那会让新进度也存不下），
            // 所以折中：把坏文件改名备份，然后从空开始 —— 历史可查、新进度能存。
            val backup = File(file.parentFile, "${file.name}.corrupt-${System.currentTimeMillis()}")
            val moved = runCatching { file.renameTo(backup) }.getOrDefault(false)
            NetDiag.log(
                "[watch] watch.json 解析失败（${e.javaClass.simpleName}: ${e.message}）；" +
                    if (moved) "已备份为 ${backup.name}，本次从空开始"
                    else "备份失败（原文件保留），本次从空开始",
            )
            progress.clear()
            dailyMs.clear()
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
        val bytes = JSONObject().put("progress", p).put("daily", d)
            .toString().toByteArray(Charsets.UTF_8)
        var out: java.io.FileOutputStream? = null
        try {
            out = disk.startWrite()
            out.write(bytes)
            disk.finishWrite(out)
        } catch (e: Exception) {
            // failWrite 会把半成品删掉、回滚到上一份完整文件。
            disk.failWrite(out)
            NetDiag.log("[watch] 写 watch.json 失败：${e.javaClass.simpleName}: ${e.message}")
        }
    }

    /** 播放器上报：位置 + 时长 + 本次新增观看毫秒。 */
    @Synchronized
    fun report(key: String, positionMs: Long, durationMs: Long, watchedDeltaMs: Long) {
        load()
        // 345 · 空键不写：否则所有"拿不到路径"的播放会共用 `""` 这一个键，
        // 与 SMB 串台（D12G）是同一类错误。宁可这条进度记不下，也不能写串。
        if (key.isNotBlank()) {
            if (positionMs > 0) {
                progress[key] = Progress(positionMs, durationMs, System.currentTimeMillis())
            } else {
                progress.remove(key)   // 看完/重置：清除续播位
            }
        }
        if (watchedDeltaMs > 0) {
            val day = java.text.SimpleDateFormat("yyyy-MM-dd",
                java.util.Locale.US).format(java.util.Date())
            dailyMs[day] = (dailyMs[day] ?: 0L) + watchedDeltaMs
        }
        persist()
    }

    /**
     * 手动删除一条在看记录（「在看」页的"移除"）。
     *
     * 只动 `progress`，**不动 `dailyMs`**：每日观看时长是既成事实的统计，
     * 用户删的是"这一条不想再看到"，不是"我没看过"。
     */
    @Synchronized
    fun remove(key: String) {
        load()
        if (progress.remove(key) != null) persist()
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
