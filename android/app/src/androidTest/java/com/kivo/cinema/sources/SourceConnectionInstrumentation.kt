package com.kivo.cinema.sources

import android.app.Activity
import android.app.Application
import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import androidx.lifecycle.ViewModelStore
import com.kivo.cinema.sources.storage.SourceCipher
import com.kivo.cinema.sources.storage.SourceKeyProvider
import com.kivo.cinema.sources.storage.SourceStore
import java.io.File
import java.security.KeyStore
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/** Deliberately late authentication results against isolated encrypted storage. */
class SourceConnectionInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }
    override fun onStart() {
        val root = File(targetContext.cacheDir, "connection-race-${System.nanoTime()}").apply { mkdirs() }
        val alias = "kivo.source.connection.test.${System.nanoTime()}"
        val repository = SourceRepository(SourceStore(File(root, "accounts.bin"), SourceCipher(SourceKeyProvider(alias))))
        val app = targetContext.applicationContext as Application
        val report = mutableListOf<String>()
        val stores = mutableListOf<ViewModelStore>()
        val releases = mutableListOf<CountDownLatch>()
        fun await(condition: () -> Boolean) {
            val deadline = SystemClock.uptimeMillis() + 8_000
            while (true) {
                var ready = false
                runOnMainSync { ready = condition() }
                if (ready) return
                check(SystemClock.uptimeMillis() < deadline)
                SystemClock.sleep(20)
            }
        }
        fun model(auth: (SourceRecord) -> Unit): SourceManagerModel {
            lateinit var result: SourceManagerModel
            runOnMainSync {
                result = SourceManagerModel(app, repository, auth)
                stores += ViewModelStore().apply { put("test", result) }
            }
            await { result.ready }
            return result
        }
        fun draft(m: SourceManagerModel, name: String, user: String) = runOnMainSync {
            m.edit(); m.name = name; m.address = "https://example.invalid"; m.username = user
            m.password = " +:synthetic secret & "
        }
        fun case(name: String, block: () -> Unit) { block(); report += "PASS $name" }
        try {
            listOf(false, true).forEach { fail ->
                case("cancelled late ${if (fail) "failure" else "success"} cannot save or overwrite a new draft") {
                    val entered = CountDownLatch(1)
                    val release = CountDownLatch(1).also { releases += it }
                    val completed = CountDownLatch(1)
                    val calls = AtomicInteger()
                    val m = model {
                        calls.incrementAndGet(); entered.countDown()
                        check(release.await(8, TimeUnit.SECONDS))
                        completed.countDown()
                        if (fail) throw SourceFailure("Synthetic late failure")
                    }
                    draft(m, "Old", "cancel-$fail")
                    runOnMainSync { m.save(); m.save() }
                    check(entered.await(5, TimeUnit.SECONDS))
                    check(calls.get() == 1)
                    runOnMainSync { check(m.verifying); m.dismiss(); check(!m.busy && !m.editing) }
                    draft(m, "New draft", "new-$fail")
                    release.countDown()
                    check(completed.await(5, TimeUnit.SECONDS))
                    // Drain delayed dispatcher delivery after the blocking authenticator returns.
                    SystemClock.sleep(250)
                    runOnMainSync {
                        check(m.editing && !m.busy && m.error == null)
                        check(m.name == "New draft" && m.username == "new-$fail")
                        check(m.password == " +:synthetic secret & ")
                    }
                    check(repository.read().getOrThrow().isEmpty())
                }
            }
            case("authentication failure preserves draft and leaves encrypted records untouched") {
                val m = model { throw SourceFailure("Synthetic rejected") }
                draft(m, "Retry draft", "retry")
                runOnMainSync { m.save() }
                await { !m.busy && m.error != null }
                runOnMainSync { check(m.editing && m.name == "Retry draft" && m.password == " +:synthetic secret & ") }
                check(repository.read().getOrThrow().isEmpty())
            }
            case("paste applies only recognized fields, preserves exact password, and never authenticates") {
                val calls = AtomicInteger()
                val m = model { calls.incrementAndGet() }
                draft(m, "Keep name", "keep-user")
                val absent = SourceShareParser.parse("example.invalid/Emby")
                val blank = SourceShareParser.parse("example.invalid/Emby\n密码：")
                val special = SourceShareParser.parse("example.invalid/Emby\n密码： +:synthetic secret & ")
                runOnMainSync {
                    m.applyPreview(absent, absent.addresses.single())
                    check(m.name == "Keep name" && m.username == "keep-user" && m.password == " +:synthetic secret & ")
                    m.applyPreview(blank, blank.addresses.single()); check(m.password == "")
                    m.applyPreview(special, special.addresses.single()); check(m.password == " +:synthetic secret & ")
                    check(!m.busy)
                }
                check(calls.get() == 0 && repository.read().getOrThrow().isEmpty())
            }
            case("successful validation commits exactly once and duplicate account is rejected before authentication") {
                val calls = AtomicInteger()
                val m = model { calls.incrementAndGet() }
                draft(m, "Saved test", "saved-test")
                runOnMainSync { m.save(); m.save() }
                await { !m.busy && !m.editing }
                val saved = repository.read().getOrThrow().single()
                check(saved.name == "Saved test" && saved.password == " +:synthetic secret & ")
                check(calls.get() == 1)
                draft(m, "Duplicate", "saved-test")
                runOnMainSync { m.save(); check(!m.busy && m.error != null && m.editing) }
                check(calls.get() == 1 && repository.read().getOrThrow().size == 1)
            }
            finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n${report.size} cases PASS\n") })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\nFAIL ${e.javaClass.simpleName}; no credentials logged\n") })
        } finally {
            releases.forEach { it.countDown() }
            runOnMainSync { stores.forEach { it.clear() } }
            root.deleteRecursively()
            KeyStore.getInstance("AndroidKeyStore").apply { load(null); deleteEntry(alias) }
        }
    }
}
