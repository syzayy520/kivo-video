package com.kivo.cinema.evidence

import com.kivo.cinema.BuildConfig
import com.kivo.cinema.net.NetDiag

/**
 * D11 取证：记录 IO 块实际执行线程与耗时到 `files/net-diag.log`。
 *
 * 单职责：只记线程/耗时，不记路径、文件名等隐私内容（沿用 NetDiag 铁律）。
 * 仅 DEBUG 构建写盘；release 下 [measure] 只是一次普通函数调用。
 * 调用点全部在 `withContext(Dispatchers.IO)` 内，真机判据是线程名以 `DefaultDispatcher-worker-` 开头。
 */
object IoTrace {
    suspend inline fun <T> measure(tag: String, crossinline block: () -> T): T {
        val started = System.nanoTime()
        try {
            return block()
        } finally {
            if (BuildConfig.DEBUG) {
                val cost = (System.nanoTime() - started) / 1_000_000L
                NetDiag.log("[io] $tag thread=${Thread.currentThread().name} cost=${cost}ms")
            }
        }
    }
}
