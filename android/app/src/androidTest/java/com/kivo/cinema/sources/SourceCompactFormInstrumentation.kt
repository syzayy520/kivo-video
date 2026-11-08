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
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Density
import android.graphics.Rect
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
class SourceCompactFormInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val root = File(targetContext.cacheDir, "source-compact-form").apply { mkdirs() }
        val alias = "kivo.source.compact.test.${System.nanoTime()}"
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
            val monitor = addMonitor(MainActivity::class.java.name, null, false)
            targetContext.startActivity(Intent(targetContext, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            host = waitForMonitorWithTimeout(monitor, 10_000) as? MainActivity ?: error("host launch timeout")
            removeMonitor(monitor)
            mainCheck {
                model = SourceManagerModel(targetContext.applicationContext as Application, repository) { calls.incrementAndGet() }
                store.put("test", model)
            }
            await { var ready = false; mainCheck { ready = model.ready }; ready }
            mainCheck {
                model.edit()
                host.setContent {
                    CompositionLocalProvider(LocalClipboardManager provides clipboard, LocalDensity provides Density(LocalDensity.current.density, 1.5f)) {
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

            stage = "real-ime"
            fun scroll(node: AccessibilityNodeInfo): Boolean {
                if (node.isScrollable && node.performAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD)) return true
                for (i in 0 until node.childCount) node.getChild(i)?.let { if (scroll(it)) return true }
                return false
            }
            repeat(5) {
                uiAutomation.rootInActiveWindow?.let { scroll(it) }
                SystemClock.sleep(150)
            }
            click("显示")
            await { find("隐藏") != null }
            click("隐藏")
            click("密码")
            await { uiAutomation.windows.any { it.type == android.view.accessibility.AccessibilityWindowInfo.TYPE_INPUT_METHOD } }
            SystemClock.sleep(600)
            repeat(3) {
                uiAutomation.rootInActiveWindow?.let { scroll(it) }
                SystemClock.sleep(150)
            }
            for (label in listOf("取消", "连接并保存", "显示")) {
                val node = find(label) ?: error("missing action")
                val bounds = Rect().also { node.getBoundsInScreen(it) }
                val window = Rect().also { uiAutomation.rootInActiveWindow.getBoundsInScreen(it) }
                check(node.isVisibleToUser && !bounds.isEmpty && window.contains(bounds)) { "action outside window" }
            }
            screenshot("phone-large-font-ime")
            report += "PASS real K40 sheet fontScale=1.5; real IME; scroll password; visibility toggle; action bounds"
            check(calls.get() == 0 && repository.read().getOrThrow().isEmpty())
            report += "PASS no authentication or persistent test accounts"
        } catch (e: Throwable) {
            val line = e.stackTrace.firstOrNull { it.fileName == "SourceCompactFormInstrumentation.kt" }?.lineNumber
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
