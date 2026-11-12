package com.kivo.cinema.sources

import android.app.Activity
import android.app.Application
import android.app.Instrumentation
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import com.kivo.cinema.MainActivity
import com.kivo.cinema.library.MediaLibraryActivity
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.storage.SourceStore

/** Real host lifecycle only. Never changes account records or verifies device credentials. */
class SourceLifecycleInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val report = mutableListOf<String>()
        var resumed: Activity? = null
        val app = targetContext.applicationContext as Application
        val callbacks = object : Application.ActivityLifecycleCallbacks {
            override fun onActivityResumed(activity: Activity) { resumed = activity }
            override fun onActivityPaused(activity: Activity) { if (resumed === activity) resumed = null }
            override fun onActivityCreated(activity: Activity, state: Bundle?) = Unit
            override fun onActivityStarted(activity: Activity) = Unit
            override fun onActivityStopped(activity: Activity) = Unit
            override fun onActivitySaveInstanceState(activity: Activity, state: Bundle) = Unit
            override fun onActivityDestroyed(activity: Activity) = Unit
        }
        fun await(condition: () -> Boolean) {
            val deadline = SystemClock.uptimeMillis() + 10_000
            while (true) {
                var ready = false
                runOnMainSync { ready = condition() }
                if (ready) return
                check(SystemClock.uptimeMillis() < deadline) { "lifecycle timeout" }
                SystemClock.sleep(50)
            }
        }
        fun shell(command: String) { uiAutomation.executeShellCommand(command).use { fd ->
            java.io.FileInputStream(fd.fileDescriptor).use { it.readBytes() }
        } }
        fun unlocked() { runOnMainSync { check(SourceAccessSession.unlocked) } }
        try {
            check(SourceStore(targetContext).read().getOrThrow().none { it.hidden }) {
                "refuse synthetic authorization while private sources exist"
            }
            runOnMainSync { app.registerActivityLifecycleCallbacks(callbacks) }
            startActivitySync(Intent(targetContext, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            await { resumed is MainActivity }
            // Wait for any default-home navigation before granting synthetic test authorization.
            SystemClock.sleep(1_000)
            await { resumed is MainActivity }
            runOnMainSync { SourceAccessSession.unlock() }
            val main = resumed!!
            // 254 D15：EmbyActivity 无 sourceId 已退役（重定向到资源库添加流），不再驻留前台。
            // 改用本地资源库页作"内部第二页"载体，验证的仍是导航往返保留前台授权这一原意。
            runOnMainSync { main.startActivity(Intent(main, MediaLibraryActivity::class.java)) }
            await { resumed is MediaLibraryActivity }
            unlocked()
            report += "PASS internal MainActivity -> MediaLibraryActivity retains foreground authorization"
            runOnMainSync { resumed!!.finish() }
            await { resumed is MainActivity }
            unlocked()
            report += "PASS internal return retains foreground authorization"

            val before = resumed!!
            runOnMainSync { before.recreate() }
            await { resumed is MainActivity && resumed !== before }
            unlocked()
            report += "PASS real activity recreation retains foreground authorization"

            repeat(3) { round ->
                runOnMainSync { SourceAccessSession.unlock() }
                shell("input keyevent KEYCODE_HOME")
                await { resumed == null && !SourceAccessSession.unlocked }
                shell("am start -n com.kivo.cinema/.MainActivity")
                await { resumed is MainActivity }
                runOnMainSync { check(!SourceAccessSession.unlocked) }
                report += "PASS desktop round ${round + 1}: background revokes authorization; return stays locked"
            }
            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", "\n${report.joinToString("\n")}\n${report.size} cases PASS\n")
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", "\n${report.joinToString("\n")}\nFAIL ${e.javaClass.simpleName}; no credentials logged\n")
            })
        } finally {
            runOnMainSync { SourceAccessSession.lock(); app.unregisterActivityLifecycleCallbacks(callbacks) }
        }
    }
}
