package com.kivo.cinema.privatezone

import android.content.Context
import org.json.JSONArray
import java.io.File

/**
 * 私密条目集合（231 设计稿 D2）：MediaStore `_ID` 字符串列表持久化到 app 私有 JSON。
 *
 * 标记与内容分离：不搬文件、不改 MediaStore；卸载即失效（v1 取舍，设计稿明示）。
 * 单一职责：集合存取——门在 privacy/PrivacyLockFlow（39 卡），UI 在 PrivateActivity/媒体库。
 */
class PrivateStore(context: Context) {

    private val file: File = File(context.filesDir, "private.json")
    private val ids = LinkedHashSet<String>()

    init {
        runCatching {
            if (file.exists()) {
                val arr = JSONArray(file.readText())
                for (i in 0 until arr.length()) ids.add(arr.getString(i))
            }
        }
    }

    @Synchronized
    fun add(id: String) {
        ids.add(id)
        persist()
    }

    @Synchronized
    fun remove(id: String) {
        ids.remove(id)
        persist()
    }

    fun contains(id: String): Boolean = ids.contains(id)

    fun all(): Set<String> = ids.toSet()

    private fun persist() {
        val arr = JSONArray()
        ids.forEach { arr.put(it) }
        file.writeText(arr.toString())
    }
}
