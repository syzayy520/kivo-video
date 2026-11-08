package com.kivo.cinema.media.marks

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

/** Real Android filesystem tests, isolated from the user's title marks. No test dependencies. */
class FilmMarksInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        val root = File(targetContext.cacheDir, "film-marks-test-${System.nanoTime()}")
        root.mkdirs()
        val report = mutableListOf<String>()
        fun case(name: String, test: () -> Unit) {
            test()
            report += "PASS $name"
        }
        try {
            val movie = FilmKey(123, false)
            val tv = FilmKey(123, true)
            val file = File(root, "normal/titles.json")
            val store = FilmMarksStore(file)
            case("independent flags, namespaces, durable reload, and removal") {
                store.refresh().getOrThrow()
                check(store.snapshots.value.ready)
                store.set(movie, FilmMark.FAVORITE, true).getOrThrow()
                store.set(movie, FilmMark.WATCHLIST, true).getOrThrow()
                store.set(movie, FilmMark.WATCHED, true).getOrThrow()
                store.set(tv, FilmMark.WATCHED, true).getOrThrow()
                val restarted = FilmMarksStore(file)
                restarted.refresh().getOrThrow()
                check(restarted.snapshots.value[movie] == FilmMarks(true, true, true))
                check(restarted.snapshots.value[tv] == FilmMarks(watched = true))
                restarted.set(movie, FilmMark.WATCHLIST, false).getOrThrow()
                store.set(tv, FilmMark.FAVORITE, true).getOrThrow()
                store.refresh().getOrThrow()
                check(store.snapshots.value[movie] == FilmMarks(true, false, true))
                check(store.snapshots.value[tv] == FilmMarks(true, false, true))
                FilmMark.entries.forEach { store.set(movie, it, false).getOrThrow() }
                check(movie.storageKey !in store.snapshots.value.entries)
                check(store.snapshots.value[tv].watched)
            }
            case("interleaved writers do not lose other titles") {
                val start = CountDownLatch(1)
                val pool = Executors.newFixedThreadPool(2)
                try {
                    val jobs = (1..16).map { id -> pool.submit {
                        start.await()
                        FilmMarksStore(file).set(FilmKey(1000 + id, false),
                            FilmMark.WATCHLIST, true).getOrThrow()
                    } }
                    start.countDown()
                    jobs.forEach { it.get(20, TimeUnit.SECONDS) }
                } finally { pool.shutdownNow() }
                store.refresh().getOrThrow()
                check((1..16).all { store.snapshots.value[FilmKey(1000 + it, false)].watchlist })
                check(store.snapshots.value[tv].favorite)
            }
            case("write failure preserves disk and confirmed state; retry succeeds") {
                val before = file.readBytes()
                // AtomicFile removes stale .new files during read. Deny directory writes instead.
                android.system.Os.chmod(file.parentFile!!.path, 320) // 0500, read/search only.
                try {
                    check(store.set(tv, FilmMark.WATCHLIST, true).isFailure)
                    check(!store.snapshots.value[tv].watchlist)
                    check(file.readBytes().contentEquals(before))
                } finally { android.system.Os.chmod(file.parentFile!!.path, 448) } // 0700
                store.set(tv, FilmMark.WATCHLIST, true).getOrThrow()
                val restarted = FilmMarksStore(file)
                restarted.refresh().getOrThrow()
                check(restarted.snapshots.value[tv].watchlist)
            }
            case("corrupt input is reported and never overwritten") {
                file.writeText("{invalid-json")
                val before = file.readBytes()
                check(store.set(tv, FilmMark.FAVORITE, false).isFailure)
                check(store.snapshots.value[tv].favorite)
                check(file.readBytes().contentEquals(before))
                check(store.refresh().isFailure)
                check(!store.snapshots.value.ready)
                check(!store.snapshots.value.error.isNullOrBlank())
            }
            case("unknown schema is rejected without modifying data") {
                file.writeText("{\"schema\":99,\"entries\":{}}")
                val before = file.readBytes()
                check(FilmMarksStore(file).refresh().isFailure)
                check(file.readBytes().contentEquals(before))
            }
            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n5/5 PASS\n"))
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\nFAIL ${e.stackTraceToString()}")
            })
        } finally { root.deleteRecursively() }
    }
}
