package com.kivo.cinema.sources

import android.app.Activity
import android.app.Application
import android.app.Instrumentation
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import android.view.accessibility.AccessibilityNodeInfo
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.ClipboardManager
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.text.AnnotatedString
import androidx.lifecycle.ViewModelStore
import com.kivo.cinema.MainActivity
import com.kivo.cinema.sources.storage.SourceCipher
import com.kivo.cinema.sources.storage.SourceKeyProvider
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.sources.ui.SourcesScreen
import java.io.File
import java.security.KeyStore
import java.util.concurrent.atomic.AtomicInteger

/** Production UI with synthetic clipboard and isolated storage; never reads the user's clipboard. */
@Suppress("DEPRECATION")
class SourcePasteUiInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val root = File(targetContext.cacheDir, "source-paste-ui").apply { mkdirs() }
        val alias = "kivo.source.paste.test.${System.nanoTime()}"
        val records = File(root, "accounts-${System.nanoTime()}.bin")
        val repository = SourceRepository(SourceStore(records, SourceCipher(SourceKeyProvider(alias))))
        val store = ViewModelStore()
        val calls = AtomicInteger()
        val report = mutableListOf<String>()
        var host: MainActivity? = null
        var stage = "start"
        lateinit var model: SourceManagerModel
        var clip = "example.invalid/Emby\n名称：Synthetic library\n账号：synthetic-user\n密码：synthetic-secret"
        val clipboard = object : ClipboardManager {
            override fun getText() = AnnotatedString(clip)
            override fun setText(annotatedString: AnnotatedString) { clip = annotatedString.text }
            override fun hasText() = clip.isNotEmpty()
        }
        fun await(condition: () -> Boolean) {
            val until = SystemClock.uptimeMillis() + 8_000
            while (!condition()) {
                check(SystemClock.uptimeMillis() < until) { "timeout" }
                SystemClock.sleep(40)
            }
        }
        fun find(text: String): AccessibilityNodeInfo? {
            fun walk(node: AccessibilityNodeInfo): AccessibilityNodeInfo? {
                if (node.text?.toString() == text) return node
                for (i in 0 until node.childCount) node.getChild(i)?.let { walk(it)?.let { return it } }
                return null
            }
            return uiAutomation.rootInActiveWindow?.let(::walk)
        }
        fun click(text: String) {
            await {
                var node = find(text)
                while (node != null && !node.isClickable) node = node.parent
                node != null && node.isEnabled && node.performAction(AccessibilityNodeInfo.ACTION_CLICK)
            }
            waitForIdleSync()
        }
        fun enabled(text: String): Boolean {
            var node = find(text)
            while (node != null && !node.isClickable) node = node.parent
            return node?.isEnabled == true
        }
        fun mainCheck(block: () -> Unit) = runOnMainSync(block)
        fun screenshot(name: String) {
            // Accessibility appears before the dialog's entrance animation has settled.
            SystemClock.sleep(400)
            uiAutomation.takeScreenshot()!!.let { bitmap ->
                File(root, "$name.png").outputStream().use { bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, it) }
                bitmap.recycle()
            }
        }
        try {
            host = startActivitySync(Intent(targetContext, MainActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) as MainActivity
            mainCheck {
                model = SourceManagerModel(targetContext.applicationContext as Application, repository) { calls.incrementAndGet() }
                store.put("test", model)
            }
            await { var ready = false; mainCheck { ready = model.ready }; ready }
            mainCheck {
                model.edit()
                host.setContent {
                    CompositionLocalProvider(LocalClipboardManager provides clipboard) {
                        Box(Modifier.fillMaxSize().background(Color(0xFFF2F2F8))) {
                            SourcesScreen(model, 0, {}, {}, {}, {}, {})
                        }
                    }
                }
            }
            stage = "sanitized-preview"
            click("粘贴识别")
            await { find("检查识别结果") != null }
            check(find("密码：已识别（隐藏）") != null && find("synthetic-secret") == null)
            mainCheck { check(model.address.isEmpty() && model.username.isEmpty()) }
            screenshot("preview")
            report += "PASS actual preview hides password and leaves draft unchanged before confirmation"
            click("填入表单")
            await { find("检查识别结果") == null }
            mainCheck { check(model.name == "Synthetic library" && model.username == "synthetic-user" && model.password == "synthetic-secret") }
            check(calls.get() == 0 && repository.read().getOrThrow().isEmpty())
            report += "PASS explicit fill updates draft without authentication or persistence"

            stage = "multiple-addresses"
            clip = "https://first.invalid\nhttps://second.invalid"
            click("粘贴识别")
            await { find("检查识别结果") != null }
            check(!enabled("填入表单"))
            click("https://second.invalid")
            click("填入表单")
            await { find("检查识别结果") == null }
            mainCheck { check(model.address == "https://second.invalid" && model.username == "synthetic-user" && model.password == "synthetic-secret") }
            report += "PASS multiple addresses require explicit selection and preserve missing account fields"

            stage = "conflicting-fields"
            clip = "https://example.invalid\n账号：first\n账号：second"
            click("粘贴识别")
            await { find("检查识别结果") != null }
            check(!enabled("填入表单"))
            screenshot("conflict")
            click("取消")
            await { find("检查识别结果") == null }
            mainCheck { check(model.username == "synthetic-user" && model.address == "https://second.invalid") }
            report += "PASS conflicting account fields block fill and cancellation preserves draft"

            stage = "exit-confirmation"
            click("取消")
            await { find("放弃未保存的修改？") != null }
            click("继续编辑")
            await { find("放弃未保存的修改？") == null }
            mainCheck { check(model.editing && model.username == "synthetic-user") }
            click("取消")
            click("放弃修改")
            await { find("粘贴识别") == null }
            mainCheck { check(!model.editing && model.password.isEmpty()) }
            check(calls.get() == 0 && repository.read().getOrThrow().isEmpty())
            report += "PASS continue editing preserves draft; discard clears credentials without saving"
        } catch (e: Throwable) {
            val line = e.stackTrace.firstOrNull { it.fileName == "SourcePasteUiInstrumentation.kt" }?.lineNumber
            report += "FAIL $stage ${e.javaClass.simpleName} line=$line; no credentials logged"
        } finally {
            mainCheck { host?.finish(); store.clear() }
            records.delete()
            KeyStore.getInstance("AndroidKeyStore").apply { load(null); deleteEntry(alias) }
        }
        finish(if (report.any { it.startsWith("FAIL") }) Activity.RESULT_CANCELED else Activity.RESULT_OK,
            Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }
}
