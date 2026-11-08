package com.kivo.cinema.sources.access

import android.app.Activity
import android.app.KeyguardManager
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts

/** OS-owned credential verification, including API26; never collects or stores the device PIN. */
class SourceUnlockController(private val activity: ComponentActivity) {
    private var success: (() -> Unit)? = null
    private var failure: ((String) -> Unit)? = null
    private val launcher = activity.registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        val done = success; val failed = failure
        success = null; failure = null
        if (result.resultCode == Activity.RESULT_OK && done != null) {
            SourceAccessSession.unlock()
            done()
        } else failed?.invoke("已取消解锁")
    }

    fun authenticate(onFailure: (String) -> Unit, onSuccess: () -> Unit) {
        if (success != null) return
        val manager = activity.getSystemService(KeyguardManager::class.java)
        @Suppress("DEPRECATION")
        val intent = if (manager?.isDeviceSecure == true)
            manager.createConfirmDeviceCredentialIntent("解锁隐藏资源库", "请验证此设备的锁屏密码") else null
        if (intent == null) {
            onFailure("请先在系统设置中启用锁屏密码，再使用资源库隐藏与解锁")
            return
        }
        success = onSuccess; failure = onFailure
        runCatching { launcher.launch(intent) }.onFailure {
            success = null; failure = null
            onFailure("无法打开系统验证，请稍后重试")
        }
    }
}
