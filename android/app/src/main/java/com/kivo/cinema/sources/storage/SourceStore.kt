package com.kivo.cinema.sources.storage

import android.content.Context
import android.util.AtomicFile
import com.kivo.cinema.sources.SourceFailure
import com.kivo.cinema.sources.SourceRecord
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/** Synchronous disk boundary; callers dispatch off the UI thread. No cached empty fallback. */
class SourceStore(private val file: File, private val cipher: SourceCipher = SourceCipher()) {
    constructor(context: Context) : this(File(context.noBackupFilesDir, "sources/accounts.bin"))

    fun read(): Result<List<SourceRecord>> = guarded { readLocked() }

    internal fun update(change: (List<SourceRecord>) -> List<SourceRecord>): Result<List<SourceRecord>> = guarded {
        val next = change(readLocked()).toList()
        val bytes = cipher.encrypt(encode(next))
        val atomic = AtomicFile(file)
        var stream: java.io.FileOutputStream? = null
        try {
            stream = atomic.startWrite()
            stream.write(bytes)
            atomic.finishWrite(stream)
            stream = null
        } finally { if (stream != null) atomic.failWrite(stream) }
        next
    }

    private fun readLocked(): List<SourceRecord> {
        val atomic = AtomicFile(file)
        if (!file.exists() && !File(file.path + ".bak").exists()) return emptyList()
        val json = JSONObject(cipher.decrypt(atomic.readFully()).toString(Charsets.UTF_8))
        if (json.getInt("schema") != 1) throw SourceFailure("源记录版本不受支持；原记录已保留")
        val records = json.getJSONArray("sources")
        val result = (0 until records.length()).map { index ->
            val item = records.getJSONObject(index)
            if (item.getString("protocol") != "emby") throw SourceFailure("源协议不受支持；原记录已保留")
            SourceRecord(item.getString("id"), item.getLong("revision"), item.getString("name"),
                item.getString("address"), item.getString("username"), item.getString("password"),
                flag(item, "hidden", false), flag(item, "defaultHome", false),
                flag(item, "aggregateEnabled", true), flag(item, "aggregateSearchEnabled", true))
        }
        require(result.map { it.id }.distinct().size == result.size)
        require(result.map { it.address to it.username }.distinct().size == result.size)
        require(result.count { it.defaultHome } <= 1)
        return result
    }

    private fun encode(records: List<SourceRecord>): ByteArray {
        val array = JSONArray()
        records.forEach { record -> array.put(JSONObject().put("id", record.id)
            .put("revision", record.revision).put("protocol", "emby").put("name", record.name)
            .put("address", record.address).put("username", record.username).put("password", record.password)
            .put("hidden", record.hidden).put("defaultHome", record.defaultHome)
            .put("aggregateEnabled", record.aggregateEnabled).put("aggregateSearchEnabled", record.aggregateSearchEnabled)) }
        return JSONObject().put("schema", 1).put("sources", array).toString().toByteArray(Charsets.UTF_8)
    }

    private fun flag(item: JSONObject, name: String, default: Boolean): Boolean {
        if (!item.has(name)) return default
        return item.get(name) as? Boolean ?: throw SourceFailure("源设置格式无效；原记录已保留")
    }

    private fun <T> guarded(block: () -> T): Result<T> = synchronized(diskLock) {
        try { Result.success(block()) }
        catch (e: SourceFailure) { Result.failure(e) }
        catch (_: Exception) { Result.failure(SourceFailure("无法读取或保存源记录；原记录已保留，请检查存储后重试")) }
    }

    private companion object { val diskLock = Any() }
}
