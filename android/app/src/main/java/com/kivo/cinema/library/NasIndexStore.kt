package com.kivo.cinema.library

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * NAS 片库索引的落盘存取（JSON）。
 *
 * 单一职责：持久化 `List<NasVideo>` + 记录上次扫描时刻；不扫描、不拼 URL、不碰 UI。
 * 阻塞（读写磁盘）：请在 IO 线程调用。
 *
 * 索引存在的意义：首页**读索引**而不是每次进页面都扫 NAS——
 * 12T 有上千条目录，扫描是分钟级的事，不能挂在首屏上。
 */
class NasIndexStore(context: Context) {

    private val file = File(File(context.filesDir, "nas"), "nas-index.json")

    fun read(): List<NasVideo> {
        if (!file.exists()) return emptyList()
        return runCatching {
            val arr = JSONObject(file.readText()).optJSONArray(KEY_VIDEOS) ?: JSONArray()
            (0 until arr.length()).mapNotNull { i ->
                val o = arr.optJSONObject(i) ?: return@mapNotNull null
                val path = o.optString("path")
                val share = o.optString("share")
                val sourceId = o.optString("sourceId")
                if (path.isBlank() || share.isBlank() || sourceId.isBlank()) return@mapNotNull null
                NasVideo(sourceId, share, path, o.optLong("size"), o.optLong("addedMs"))
            }
        }.getOrElse { emptyList() }
    }

    fun write(videos: List<NasVideo>) {
        runCatching {
            file.parentFile?.mkdirs()
            val arr = JSONArray()
            videos.forEach { v ->
                arr.put(
                    JSONObject().put("sourceId", v.sourceId).put("share", v.share)
                        .put("path", v.path).put("size", v.size).put("addedMs", v.addedMs),
                )
            }
            file.writeText(
                JSONObject().put("schema", SCHEMA).put("scanned_at", System.currentTimeMillis())
                    .put(KEY_VIDEOS, arr).toString(),
            )
        }
    }

    /** 上次成功扫描的时刻（0 = 从未扫过）。 */
    fun scannedAtMs(): Long {
        if (!file.exists()) return 0L
        return runCatching { JSONObject(file.readText()).optLong("scanned_at", 0L) }.getOrElse { 0L }
    }

    private companion object {
        const val SCHEMA = 1
        const val KEY_VIDEOS = "videos"
    }
}
