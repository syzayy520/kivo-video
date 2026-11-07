package com.kivo.cinema.privatezone

import android.content.Context
import android.content.DialogInterface
import android.hardware.biometrics.BiometricManager
import android.hardware.biometrics.BiometricPrompt
import android.os.CancellationSignal

/**
 * 私密区门（231 设计稿 D1）：平台 BiometricPrompt 封装，零第三方。
 *
 * 选型注（2026-11-08）：SDK 37 的 Builder 方法面是 setNegativeButton(CharSequence, Executor,
 * Listener)——老教程里的 setNegativeButtonText 不存在；deprecated 的 FingerprintManager 类
 * 在 SDK 37 存根里已被移除。均 javap 实证。
 * 每次进入都验证（对齐 Rex "unlock protected viewing environments" 语义）。
 */
class PrivateGate {

    sealed class Result {
        object Success : Result()
        data class Failed(val reason: String) : Result()   // G9：reason 用户可见
    }

    /** 设备是否具备生物特征（录入且可用）。 */
    fun available(ctx: Context): Boolean {
        val bm = ctx.getSystemService(BiometricManager::class.java) ?: return false
        return bm.canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_WEAK) ==
            BiometricManager.BIOMETRIC_SUCCESS
    }

    /** 弹系统验证窗。回调线程 = [activity.mainExecutor]。 */
    fun authenticate(activity: android.app.Activity,
                     title: String,
                     onResult: (Result) -> Unit) {
        val prompt = BiometricPrompt.Builder(activity)
            .setTitle(title)
            .setNegativeButton("取消", activity.mainExecutor,
                DialogInterface.OnClickListener { _, _ ->
                    onResult(Result.Failed("已取消"))
                })
            .build()
        prompt.authenticate(CancellationSignal(), activity.mainExecutor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(
                    result: BiometricPrompt.AuthenticationResult?) {
                    onResult(Result.Success)
                }

                override fun onAuthenticationFailed() {
                    // 单次指纹不匹配：系统窗内自动重试，不外抛
                }

                override fun onAuthenticationError(code: Int, errString: CharSequence) {
                    onResult(Result.Failed(errString.toString()))
                }
            })
    }
}
