package com.kivo.cinema.sources

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import com.kivo.cinema.sources.storage.SourceCipher
import com.kivo.cinema.sources.storage.SourceKeyProvider
import com.kivo.cinema.sources.storage.SourceStore
import java.io.File
import java.security.KeyStore
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

/** Real Keystore/filesystem tests; only synthetic accounts and disposable storage/aliases. */
class SourceStorageInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val root = File(targetContext.cacheDir, "source-tests-${System.nanoTime()}").apply { mkdirs() }
        val alias = "kivo.sources.test.${System.nanoTime()}"
        val keys = SourceKeyProvider(alias)
        val cipher = SourceCipher(keys)
        val file = File(root, "records/accounts.bin")
        fun repository() = SourceRepository(SourceStore(file, cipher))
        val report = mutableListOf<String>()
        fun case(name: String, block: () -> Unit) { block(); report += "PASS $name" }
        fun record(id: String, revision: Long = 1, password: String = "synthetic +:& 密码 ") =
            SourceRecord(id, revision, "Test $id", "https://example.invalid/Emby", id, password)
        fun failed(result: Result<*>) {
            check(result.isFailure)
            val reason = result.exceptionOrNull()?.message.orEmpty()
            check(reason.isNotBlank() && !reason.contains("synthetic"))
        }
        try {
            case("normalize default ports; preserve path case; reject credential/query URLs") {
                check(SourceAddress.normalize("EXAMPLE.invalid:443/Emby/") == "https://example.invalid/Emby")
                check(SourceAddress.normalize("http://example.invalid:80/a") == "http://example.invalid/a")
                check(SourceAddress.normalize("https://[::1]:443/a") == "https://[::1]/a")
                listOf("https://user:secret@example.invalid", "https://example.invalid?token=secret",
                    "ftp://example.invalid", "https://example.invalid:70000", "https://example.invalid/a/../b")
                    .forEach { check(runCatching { SourceAddress.normalize(it) }.isFailure) }
            }
            case("paste labels preserve password spaces and punctuation; blank password distinct from absent") {
                val p = SourceShareParser.parse("名称：测试\n地址：example.invalid/Emby\n用户名：alice\n密码： +:密 码 & ")
                check(p.canApply && p.name == "测试" && p.username == "alice" && p.password == " +:密 码 & ")
                check(SourceShareParser.parse("example.invalid\n密码：").password == "")
                check(SourceShareParser.parse("example.invalid").password == null)
                check(!p.toString().contains("alice"))
            }
            case("URL userinfo decoded without corrupting plus; removed from address") {
                val p = SourceShareParser.parse("https://alice:p%3A%26+%20@example.invalid:443/Emby")
                check(p.canApply && p.username == "alice" && p.password == "p:&+ ")
                check(p.addresses.single() == "https://example.invalid/Emby")
            }
            case("multiple URLs, conflicting fields and ports require review") {
                check(!SourceShareParser.parse("https://one.invalid\nhttps://two.invalid").canApply)
                check(!SourceShareParser.parse("example.invalid\n账号：a\n账号：b").canApply)
                check(!SourceShareParser.parse("https://example.invalid:443\n端口：8096").canApply)
                check(!SourceShareParser.parse("example.invalid:443\n端口：8096").canApply)
                check(SourceShareParser.parse("example.invalid/Emby\n端口：8096").addresses.single() ==
                    "https://example.invalid:8096/Emby")
                check(!SourceShareParser.parse("some prose without an address").canApply)
            }
            case("encrypted reload, blank password, secret-safe string, and randomized ciphertext") {
                check(repository().read().getOrThrow().isEmpty())
                repository().saveVerified(record("alpha")).getOrThrow()
                repository().saveVerified(record("beta", password = "")).getOrThrow()
                val loaded = repository().read().getOrThrow()
                check(loaded.size == 2 && loaded[0].password == "synthetic +:& 密码 " && loaded[1].password == "")
                check(loaded[0].toString() == "SourceRecord(redacted)")
                val disk = file.readBytes()
                check(!disk.toString(Charsets.ISO_8859_1).contains("synthetic"))
                val a = cipher.encrypt("same input".toByteArray())
                val b = cipher.encrypt("same input".toByteArray())
                check(!a.contentEquals(b) && cipher.decrypt(a).contentEquals(cipher.decrypt(b)))
            }
            case("duplicate rejected; different accounts allowed; stale edit and deletion protected") {
                val before = file.readBytes()
                failed(repository().saveVerified(SourceRecord(name = "duplicate", address = "https://EXAMPLE.invalid:443/Emby/",
                    username = "alpha", password = "")))
                check(file.readBytes().contentEquals(before))
                repository().saveVerified(record("alpha", revision = 2), 1).getOrThrow()
                failed(repository().saveVerified(record("alpha", revision = 2), 1))
                failed(repository().remove("alpha", 1))
                check(repository().read().getOrThrow().first().revision == 2L)
            }
            case("interleaved repository writers preserve every source") {
                val start = CountDownLatch(1)
                val pool = Executors.newFixedThreadPool(3)
                try {
                    val jobs = (1..12).map { id -> pool.submit {
                        start.await()
                        repository().saveVerified(record("parallel-$id")).getOrThrow()
                    } }
                    start.countDown()
                    jobs.forEach { it.get(30, TimeUnit.SECONDS) }
                } finally { pool.shutdownNow() }
                check(repository().read().getOrThrow().size == 14)
            }
            case("failed disk write preserves old configuration and permits retry") {
                val before = file.readBytes()
                android.system.Os.chmod(file.parentFile!!.path, 320)
                try {
                    failed(repository().saveVerified(record("alpha", revision = 3), 2))
                    check(file.readBytes().contentEquals(before))
                    check(repository().read().getOrThrow().first().revision == 2L)
                } finally { android.system.Os.chmod(file.parentFile!!.path, 448) }
                repository().saveVerified(record("alpha", revision = 3), 2).getOrThrow()
            }
            case("authenticated corruption and unknown schemas are visible and never overwritten") {
                val original = file.readBytes()
                val corrupt = original.copyOf().apply { this[lastIndex] = (this[lastIndex].toInt() xor 1).toByte() }
                file.writeBytes(corrupt)
                failed(repository().read())
                failed(repository().saveVerified(record("blocked")))
                check(file.readBytes().contentEquals(corrupt))
                val unknown = cipher.encrypt("{\"schema\":999,\"sources\":[]}".toByteArray())
                file.writeBytes(unknown)
                failed(repository().read())
                failed(repository().remove("alpha", 3))
                check(file.readBytes().contentEquals(unknown))
                file.writeBytes(original)
            }
            case("missing Keystore key cannot regenerate or discard saved accounts") {
                val before = file.readBytes()
                KeyStore.getInstance("AndroidKeyStore").apply { load(null); deleteEntry(alias) }
                check(keys.existing() == null)
                failed(repository().read())
                failed(repository().saveVerified(record("blocked")))
                check(keys.existing() == null && file.readBytes().contentEquals(before))
            }
            finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n${report.size} cases PASS\n") })
        } catch (_: Throwable) {
            // Test values are synthetic, but never teach production to log throwable text with secrets.
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", "\n${report.joinToString("\n")}\nFAIL case ${report.size + 1}; inspect corresponding assertions\n")
            })
        } finally {
            KeyStore.getInstance("AndroidKeyStore").apply { load(null); deleteEntry(alias) }
            root.deleteRecursively()
        }
    }
}
