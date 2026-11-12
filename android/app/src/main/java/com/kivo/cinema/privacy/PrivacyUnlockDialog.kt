package com.kivo.cinema.privacy

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.kivo.cinema.sources.access.SourceAccessSession

/**
 * 资源库 / Emby 的隐私门弹窗（39 卡）。单一职责：把 [PrivacyLockFlow] 包成全屏门，
 * 无密码先首设、有密码则解锁，成功即放行 [SourceAccessSession]。
 * 两个调用方：MainActivity（资源库锁图标 / 隐藏开关）、EmbyActivity（锁定态门）。
 */
@Composable
fun PrivacyUnlockDialog(onDismiss: () -> Unit, onUnlocked: () -> Unit) {
    val context = LocalContext.current
    val password = remember { PrivacyPassword(context) }
    Dialog(onDismissRequest = onDismiss, properties = DialogProperties(usePlatformDefaultWidth = false)) {
        MaterialTheme(colorScheme = lightColorScheme(
            primary = Color(0xFF238A59), background = Color(0xFFF5F4F8),
            surface = Color.White, onSurface = Color(0xFF20222A), onSurfaceVariant = Color(0xFF70727F),
        )) {
            Box(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background)) {
                PrivacyLockFlow(
                    password = password,
                    mode = if (password.isSet) PrivacyLockMode.UNLOCK else PrivacyLockMode.SETUP,
                    onDone = { SourceAccessSession.unlock(); onUnlocked() },
                    onCancel = onDismiss,
                )
            }
        }
    }
}