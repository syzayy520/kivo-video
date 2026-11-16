package com.kivo.cinema.stats

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import org.json.JSONObject
import java.io.File

/**
 * 307 · `WatchStore` 原子写验收。
 *
 * ## 为什么必须测
 * 原实现是 `file.writeText()` 直接覆写：写到一半掉电/崩溃就留下半个 JSON，
 * 下次 `load()` 解析失败**而且异常被 runCatching 吞掉** ⇒ 用户看到的是
 * "观看进度全没了"，我们连原因都没有。watch.json 是**每秒级高频写**的文件，
 * 这条风险不是理论上的 —— 它每天都在被写。
 *
 * ## 三个用例
 * 1. **正常写读**：report 后新实例能读回同样的位置；
 * 2. **损坏恢复**：塞一个半截 JSON ⇒ 不崩、**备份留下**、新进度能存能读；
 * 3. **连续写 50 次，每次写完立刻解析**：文件在任何时刻都是完整 JSON
 *    （原子写的直接表现 —— 旧实现会出现"正好读到半个文件"的窗口）。
 *
 * ## 为什么用独立文件名
 * 构造参数 `fileName` 传 `watch.test.json` ⇒ **不碰用户真实的观看进度**（`watch.json`）。
 */
class WatchStoreAtomicInstrumentation : Instrumentation() {

    private val results = mutableListOf<Pair<String, Boolean>>()
    private val testFile = "watch.test.json"

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        val dir = targetContext.filesDir
        cleanup(dir)

        // ---- 1：正常写读 ----
        runCatching {
            val store = WatchStore(targetContext, testFile)
            store.report("k1", 60_000L, 600_000L, 5_000L)
            val reloaded = WatchStore(targetContext, testFile)
            val p = reloaded.progressOf("k1")
            results += "1 正常写读（pos=${p?.positionMs}）" to (p?.positionMs == 60_000L)
        }.onFailure { results += "1 正常写读（异常 ${it}）" to false }

        // ---- 2：损坏恢复 ----
        runCatching {
            // 塞一个"写到一半"的 JSON（真机上掉电就是这个样）
            File(dir, testFile).writeText("{\"progress\": {\"k2\": {\"pos\": 12")
            val store = WatchStore(targetContext, testFile)   // load 时解析失败
            store.report("k3", 30_000L, 300_000L, 0L)         // 必须还能写
            val again = WatchStore(targetContext, testFile)
            val p = again.progressOf("k3")
            val backups = dir.listFiles { f -> f.name.startsWith("$testFile.corrupt-") }
            results += "2a 损坏后不崩且新进度可读" to (p?.positionMs == 30_000L)
            results += "2b 坏文件留下备份（${backups?.size ?: 0} 份）" to (!backups.isNullOrEmpty())
        }.onFailure { results += "2 损坏恢复（异常 ${it}）" to false }

        // ---- 3：连续写 50 次，每次写完立即解析 ----
        runCatching {
            cleanupBackups(dir)
            val store = WatchStore(targetContext, testFile)
            var allParseOk = true
            var brokenAt = -1
            repeat(50) { i ->
                store.report("seq$i", i * 1_000L, 600_000L, 100L)
                val text = File(dir, testFile).readText()
                val ok = runCatching { JSONObject(text) }.isSuccess
                if (!ok) { allParseOk = false; if (brokenAt < 0) brokenAt = i }
            }
            results += "3 连续写 50 次，文件始终完整" to allParseOk
            if (!allParseOk) results += "  首个坏在第 $brokenAt 次" to false
        }.onFailure { results += "3 连续写（异常 ${it}）" to false }

        cleanup(dir)

        val pass = results.count { it.second }
        val sb = StringBuilder()
        results.forEach { (name, ok) -> sb.append(if (ok) "PASS " else "FAIL ").append(name).append('\n') }
        sb.append("$pass/${results.size} PASS\n")
        finish(
            if (pass == results.size) Activity.RESULT_OK else Activity.RESULT_CANCELED,
            Bundle().apply {
                putString("stream", sb.toString())
                putInt("pass", pass)
                putInt("total", results.size)
            },
        )
    }

    /** 清掉测试文件与它可能产生的原子写临时文件/备份。 */
    private fun cleanup(dir: File) {
        cleanupBackups(dir)
        File(dir, testFile).delete()
        // AtomicFile 的中间文件与回滚文件
        File(dir, "$testFile.new").delete()
        File(dir, "$testFile.bak").delete()
    }

    private fun cleanupBackups(dir: File) {
        dir.listFiles { f -> f.name.startsWith("$testFile.corrupt-") }?.forEach { it.delete() }
    }
}
