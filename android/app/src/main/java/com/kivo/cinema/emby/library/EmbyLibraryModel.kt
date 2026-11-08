package com.kivo.cinema.emby.library

import android.os.Handler
import android.os.Looper
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyItem
import java.util.concurrent.Executors
import java.util.concurrent.Future

/** Main-thread list state. A generation fences every delayed or asynchronous reply. */
class EmbyLibraryModel(private val errorText: (Throwable) -> String) {
    var items by mutableStateOf(emptyList<EmbyItem>()); private set
    var total by mutableStateOf<Int?>(null); private set
    var query by mutableStateOf(""); private set
    var sort by mutableStateOf(0); private set
    var loading by mutableStateOf(false); private set
    var error by mutableStateOf<String?>(null); private set
    var hasMore by mutableStateOf(false); private set
    private var client: EmbyClient? = null
    private var auth: EmbyClient.Auth? = null
    private var offset = 0
    private var generation = 0
    private val main = Handler(Looper.getMainLooper())
    private val pool = Executors.newFixedThreadPool(2)
    private var work: Future<*>? = null
    private var pending: Runnable? = null

    fun attach(c: EmbyClient, a: EmbyClient.Auth, page: EmbyClient.Page) {
        clear(); client = c; auth = a; accept(page)
    }

    fun search(value: String) {
        if (query == value) return
        query = value
        reset(350)
    }

    fun sortBy(value: Int) {
        if (sort == value) return
        sort = value
        reset(0)
    }

    private fun reset(delay: Long) {
        invalidate()
        items = emptyList(); offset = 0; total = null; error = null; hasMore = true
        loading = true
        pending = Runnable { pending = null; loading = false; loadMore() }
        main.postDelayed(pending!!, delay)
    }

    fun loadMore() {
        if (loading || !hasMore) return
        val c = client ?: return
        val a = auth ?: return
        val ticket = generation
        val start = offset
        val term = query.trim()
        val ordering = sort
        loading = true; error = null
        work = pool.submit {
            val result = runCatching { c.items(a, start, term, ordering) }
            main.post {
                if (ticket != generation) return@post
                loading = false
                result.fold(onSuccess = { accept(it) }, onFailure = { error = errorText(it) })
            }
        }
    }

    private fun accept(page: EmbyClient.Page) {
        offset += page.items.size
        items = (items + page.items).distinctBy { it.id }
        total = page.total
        hasMore = page.items.isNotEmpty() && (page.total?.let { offset < it } ?: (page.items.size == 60))
    }

    private fun invalidate() {
        ++generation; work?.cancel(true)
        pending?.let { main.removeCallbacks(it) }; pending = null
    }

    fun clear() {
        invalidate(); client = null; auth = null; items = emptyList()
        total = null; offset = 0; query = ""; sort = 0
        loading = false; error = null; hasMore = false
    }

    fun close() { clear(); pool.shutdownNow() }
}
