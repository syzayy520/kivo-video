package com.kivo.cinema.sources

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.access.SourceEligibility
import com.kivo.cinema.sources.storage.SourceCipher
import com.kivo.cinema.sources.storage.SourceKeyProvider
import com.kivo.cinema.sources.storage.SourceStore
import org.json.JSONObject
import java.io.File
import java.security.KeyStore

/** Isolated real Keystore tests; never reads/writes the user's source store or device PIN. */
class SourceControlsInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }
    override fun onStart() {
        val root = File(targetContext.cacheDir, "source-controls-${System.nanoTime()}").apply { mkdirs() }
        val file = File(root, "accounts.bin")
        val alias = "kivo.source.controls.test.${System.nanoTime()}"
        val cipher = SourceCipher(SourceKeyProvider(alias))
        fun repo() = SourceRepository(SourceStore(file, cipher))
        fun source(id: String, revision: Long = 1) = SourceRecord(id, revision, "Synthetic $id",
            "https://example.invalid", id, "test secret")
        fun current(id: String) = repo().read().getOrThrow().single { it.id == id }
        fun set(id: String, control: SourceRepository.Control, value: Boolean) =
            repo().setControl(id, current(id).revision, control, value).getOrThrow()
        val report = mutableListOf<String>()
        fun case(name: String, check: () -> Unit) { check(); report += "PASS $name" }
        try {
            case("legacy schema1 accounts retain credentials and safe defaults") {
                val json = """{"schema":1,"sources":[{"id":"a","revision":1,"name":"Synthetic a","protocol":"emby","address":"https://example.invalid","username":"a","password":"test secret"}]}"""
                file.writeBytes(cipher.encrypt(json.toByteArray()))
                val a = current("a")
                check(!a.hidden && !a.defaultHome && a.aggregateEnabled && a.aggregateSearchEnabled)
                check(a.password == "test secret")
                repo().saveVerified(source("b")).getOrThrow()
            }
            case("default home is unique; replacing it invalidates previous editor revision") {
                set("a", SourceRepository.Control.DEFAULT_HOME, true)
                val old = current("a")
                set("b", SourceRepository.Control.DEFAULT_HOME, true)
                check(!current("a").defaultHome && current("b").defaultHome)
                check(repo().saveVerified(source("a", old.revision + 1), old.revision).isFailure)
            }
            case("home and search aggregation are independent and survive reload") {
                set("a", SourceRepository.Control.AGGREGATE_SEARCH, false)
                check(current("a").aggregateEnabled && !current("a").aggregateSearchEnabled)
                check(SourceEligibility.home(repo().read().getOrThrow()).any { it.id == "a" })
                check(SourceEligibility.search(repo().read().getOrThrow()).none { it.id == "a" })
                set("b", SourceRepository.Control.AGGREGATE, false)
                check(SourceEligibility.search(repo().read().getOrThrow()).any { it.id == "b" })
                check(SourceEligibility.home(repo().read().getOrThrow()).none { it.id == "b" })
            }
            case("hidden source excluded from aggregation even in an unlocked foreground session") {
                set("b", SourceRepository.Control.HIDDEN, true)
                val b = current("b")
                check(!SourceEligibility.visible(b, false) && SourceEligibility.visible(b, true))
                check(SourceEligibility.home(listOf(b)).isEmpty() && SourceEligibility.search(listOf(b)).isEmpty())
                runOnMainSync {
                    SourceAccessSession.unlock()
                    check(SourceAccessSession.unlocked)
                    SourceAccessSession.lock()
                    check(!SourceAccessSession.unlocked)
                }
            }
            case("verified account edits preserve hidden/default/home/search preferences") {
                val b = current("b")
                repo().saveVerified(source("b", b.revision + 1), b.revision).getOrThrow()
                val updated = current("b")
                check(updated.hidden && updated.defaultHome && !updated.aggregateEnabled && updated.aggregateSearchEnabled)
            }
            case("stale preference write leaves bytes intact; all settings can be reversed") {
                val a = current("a")
                set("a", SourceRepository.Control.AGGREGATE, false)
                val before = file.readBytes()
                check(repo().setControl("a", a.revision, SourceRepository.Control.HIDDEN, true).isFailure)
                check(before.contentEquals(file.readBytes()))
                set("a", SourceRepository.Control.AGGREGATE, true)
                set("a", SourceRepository.Control.AGGREGATE_SEARCH, true)
                set("b", SourceRepository.Control.HIDDEN, false)
                set("b", SourceRepository.Control.DEFAULT_HOME, false)
                check(repo().read().getOrThrow().none { it.hidden || it.defaultHome })
                set("b", SourceRepository.Control.DEFAULT_HOME, true)
                repo().remove("b", current("b").revision).getOrThrow()
                check(repo().read().getOrThrow().none { it.defaultHome })
            }
            case("malformed privacy flags fail closed without overwriting the file") {
                val original = file.readBytes()
                val json = JSONObject(cipher.decrypt(original).toString(Charsets.UTF_8))
                json.getJSONArray("sources").getJSONObject(0).put("hidden", "false")
                val malformed = cipher.encrypt(json.toString().toByteArray())
                file.writeBytes(malformed)
                check(repo().read().isFailure)
                check(repo().saveVerified(source("c")).isFailure)
                check(malformed.contentEquals(file.readBytes()))
                file.writeBytes(original)
            }
            finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n${report.size} cases PASS\n") })
        } catch (_: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\nFAIL case ${report.size + 1}\n") })
        } finally {
            runOnMainSync { SourceAccessSession.lock() }
            KeyStore.getInstance("AndroidKeyStore").apply { load(null); deleteEntry(alias) }
            root.deleteRecursively()
        }
    }
}
