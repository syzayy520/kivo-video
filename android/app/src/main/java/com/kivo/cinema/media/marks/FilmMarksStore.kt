package com.kivo.cinema.media.marks

import android.content.Context
import android.util.AtomicFile
import java.io.File
import java.io.FileNotFoundException
import java.io.IOException
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.json.JSONObject

/** Atomic local title marks. Call disk operations on an IO dispatcher. */
class FilmMarksStore internal constructor(file: File) {
    private val disk = AtomicFile(file)
    private val mutable = MutableStateFlow(FilmMarksSnapshot())
    val snapshots = mutable.asStateFlow()

    fun refresh(): Result<Unit> = synchronized(diskLock) {
        runCatching { mutable.value = FilmMarksSnapshot(read(), ready = true) }
            .onFailure { e ->
                mutable.value = mutable.value.copy(ready = false, error = reason(e))
            }
    }

    fun set(key: FilmKey, mark: FilmMark, enabled: Boolean): Result<FilmMarks> =
        synchronized(diskLock) {
            runCatching {
                val entries = read().toMutableMap()
                val next = (entries[key.storageKey] ?: FilmMarks()).with(mark, enabled)
                if (next == FilmMarks()) entries.remove(key.storageKey)
                else entries[key.storageKey] = next
                write(entries)
                mutable.value = FilmMarksSnapshot(entries.toMap(), ready = true)
                next
            }
        }

    private fun read(): Map<String, FilmMarks> {
        val bytes = try { disk.readFully() } catch (e: FileNotFoundException) {
            if (!disk.baseFile.exists() && !File(disk.baseFile.path + ".bak").exists())
                return emptyMap()
            throw e
        }
        val root = JSONObject(bytes.toString(Charsets.UTF_8))
        require(root.getInt("schema") == 1) { "不支持的影片标记版本" }
        val rows = root.getJSONObject("entries")
        return buildMap {
            for (key in rows.keys()) {
                require(key.matches(Regex("(movie|tv):[1-9][0-9]*"))) { "影片标记身份无效" }
                val row = rows.getJSONObject(key)
                put(key, FilmMarks(row.getBoolean("favorite"), row.getBoolean("watchlist"),
                    row.getBoolean("watched")))
            }
        }
    }

    private fun write(entries: Map<String, FilmMarks>) {
        val rows = JSONObject()
        entries.forEach { (key, marks) ->
            rows.put(key, JSONObject().put("favorite", marks.favorite)
                .put("watchlist", marks.watchlist).put("watched", marks.watched))
        }
        val bytes = JSONObject().put("schema", 1).put("entries", rows)
            .toString().toByteArray(Charsets.UTF_8)
        val stream = disk.startWrite()
        try {
            stream.write(bytes)
            stream.fd.sync()
            disk.finishWrite(stream)
            // AtomicFile logs some filesystem errors instead of throwing them.
            if (!disk.readFully().contentEquals(bytes)) throw IOException("落盘校验失败")
        } catch (e: Exception) {
            disk.failWrite(stream)
            throw e
        }
    }

    private fun reason(e: Throwable): String =
        "读取影片标记失败：${e.message?.takeIf { it.isNotBlank() } ?: e.javaClass.simpleName}"

    companion object {
        private val diskLock = Any()
        @Volatile private var instance: FilmMarksStore? = null

        fun get(context: Context): FilmMarksStore = instance ?: synchronized(diskLock) {
            instance ?: FilmMarksStore(File(context.applicationContext.filesDir,
                "media/marks/titles.json")).also { instance = it }
        }
    }
}
