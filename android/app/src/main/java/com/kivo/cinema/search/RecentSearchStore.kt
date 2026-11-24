package com.kivo.cinema.search

import android.content.Context

/**
 * 最近搜索的持久化（SharedPreferences，换行分隔；词内不许有换行）。
 * 单一职责：读取 / 置顶去重追加 / 清空。上限 10 条，最新在前。
 */
class RecentSearchStore(context: Context) {

    private val prefs = context.applicationContext
        .getSharedPreferences("search_recent", Context.MODE_PRIVATE)

    fun list(): List<String> =
        prefs.getString(KEY, "").orEmpty().split('\n').filter { it.isNotBlank() }

    /** 搜索提交/点结果时记录：去重置顶，裁剪到 10 条。 */
    fun add(term: String) {
        val clean = term.replace('\n', ' ').trim()
        if (clean.isEmpty()) return
        val next = (listOf(clean) + list().filter { it != clean }).take(MAX)
        prefs.edit().putString(KEY, next.joinToString("\n")).apply()
    }

    fun clear() = prefs.edit().remove(KEY).apply()

    private companion object {
        const val KEY = "terms"
        const val MAX = 10
    }
}