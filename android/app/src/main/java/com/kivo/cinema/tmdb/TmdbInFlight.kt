package com.kivo.cinema.tmdb

import android.os.SystemClock
import com.kivo.cinema.net.NetDiag
import java.util.concurrent.Callable
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ExecutionException
import java.util.concurrent.FutureTask

/**
 * D13 · TMDB 请求闸门：同 URL 单飞（in-flight 合并）+ 一行可观测日志。
 *
 * 单职责：只合并"响应正文"，不解析 JSON、不落盘、不决定 TTL。
 * 这样每个调用方仍各自做 HTTP 状态判断与错误分类，语义零漂移
 * （非 2xx 由调用方按自身逻辑归 "server"，异常按 [java.net] 类型分类）。
 *
 * 为什么用 [FutureTask] 而不是 `Deferred`：调用栈是阻塞式的（`HttpURLConnection`），
 * `Deferred` 需要 `CoroutineScope`/`runBlocking`，会把 IO 线程锁死且丢掉取消语义。
 *
 * 进程级单例：`TmdbPeople/Providers/Genres/Studios` 每次都 `new TmdbHome(client)`，
 * 表若挂在实例上，跨调用方的重复请求一个也合并不了。
 */
internal object TmdbInFlight {
    private val tasks = ConcurrentHashMap<String, FutureTask<String?>>()

    /** [fetch] 返回响应正文；非 2xx 返回 null；异常原样抛给调用方。 */
    fun body(url: String, tag: String, fetch: () -> String?): String? {
        tasks[url]?.let { running ->
            NetDiag.log("[tmdb-net] JOIN $tag")
            return await(running)
        }
        val task = FutureTask(Callable { fetch() })
        val leader = tasks.putIfAbsent(url, task)
        if (leader != null) {
            NetDiag.log("[tmdb-net] JOIN $tag")
            return await(leader)
        }
        try {
            val started = SystemClock.elapsedRealtime()
            NetDiag.log("[tmdb-net] START $tag thread=${Thread.currentThread().name}")
            task.run()
            val body = await(task)
            NetDiag.log("[tmdb-net] DONE ${SystemClock.elapsedRealtime() - started}ms $tag")
            return body
        } finally {
            tasks.remove(url, task)
        }
    }

    /** 解包 [ExecutionException]：否则 `kindOf()` 会把超时/DNS 失败全归成 "lost"（静默降级）。 */
    private fun await(task: FutureTask<String?>): String? = try {
        task.get()
    } catch (e: ExecutionException) {
        throw (e.cause as? Exception) ?: e
    }
}
