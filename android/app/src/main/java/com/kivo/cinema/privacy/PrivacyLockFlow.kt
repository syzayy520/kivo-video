package com.kivo.cinema.privacy

import androidx.compose.animation.Crossfade
import androidx.compose.animation.core.tween
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/** 数字密码流程的三种用途（39 卡）。 */
enum class PrivacyLockMode { UNLOCK, SETUP, CHANGE }

/**
 * 设置 / 解锁 / 改密三态流程编排（39 卡）。单一职责：把 [PrivacyKeypad] 的输入
 * 按用途推进成步骤，并调 [PrivacyPassword] 落库 / 校验。不持有解锁会话——
 * 成功后由调用方决定放行（私密区扫描 / SourceAccessSession.unlock）。
 */
@Composable
fun PrivacyLockFlow(
    password: PrivacyPassword,
    mode: PrivacyLockMode,
    onDone: () -> Unit,
    onCancel: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var step by remember { mutableStateOf(0) }
    var entered by remember { mutableStateOf("") }
    var firstPin by remember { mutableStateOf("") }
    var oldPin by remember { mutableStateOf("") }
    var error by remember { mutableStateOf<String?>(null) }
    var shakeTick by remember { mutableStateOf(0) }
    var showForgot by remember { mutableStateOf(false) }

    fun fail(message: String) {
        error = message; shakeTick++; entered = ""
    }
    fun advance() {
        error = null; entered = ""
    }
    fun submit() {
        when (mode) {
            PrivacyLockMode.UNLOCK ->
                if (password.verify(entered)) { error = null; onDone() } else fail("密码不正确")
            PrivacyLockMode.SETUP -> when (step) {
                0 -> if (entered.length !in 4..6) fail("密码须为 4–6 位数字")
                     else { firstPin = entered; advance(); step = 1 }
                else -> if (entered != firstPin) { fail("两次输入不一致"); step = 0; firstPin = "" }
                        else password.set(entered).fold({ error = null; onDone() }, { fail(it.message ?: "设置失败") })
            }
            PrivacyLockMode.CHANGE -> when (step) {
                0 -> if (password.verify(entered)) { oldPin = entered; advance(); step = 1 } else fail("原密码不正确")
                1 -> if (entered.length !in 4..6) fail("密码须为 4–6 位数字")
                     else { firstPin = entered; advance(); step = 2 }
                else -> if (entered != firstPin) { fail("两次输入不一致"); step = 1; firstPin = "" }
                        else password.change(oldPin, entered).fold({ error = null; onDone() }, { fail(it.message ?: "修改失败") })
            }
        }
    }

    val title = when (mode) {
        PrivacyLockMode.UNLOCK -> "输入私密密码"
        PrivacyLockMode.SETUP -> if (step == 0) "设置私密密码" else "确认密码"
        PrivacyLockMode.CHANGE -> when (step) { 0 -> "修改密码"; 1 -> "新密码"; else -> "确认新密码" }
    }
    val subtitle = when (mode) {
        PrivacyLockMode.UNLOCK -> "解锁私密区与隐藏资源库"
        PrivacyLockMode.SETUP -> if (step == 0) "输入 4–6 位数字，独立于手机锁屏" else "再次输入以确认"
        PrivacyLockMode.CHANGE -> when (step) { 0 -> "先输入原密码"; 1 -> "输入 4–6 位数字"; else -> "再次输入以确认" }
    }

    Column(
        modifier.fillMaxSize().padding(horizontal = 32.dp, vertical = 40.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Crossfade(targetState = step, animationSpec = tween(220), label = "step") { _ ->
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(title, fontSize = 21.sp, fontWeight = FontWeight.SemiBold, color = MaterialTheme.colorScheme.onSurface)
                Spacer(Modifier.height(8.dp))
                Text(subtitle, fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant, textAlign = TextAlign.Center)
            }
        }
        Spacer(Modifier.height(36.dp))
        PrivacyKeypad(
            entered = entered, maxLength = 6, error = error != null, shakeTick = shakeTick,
            onDigit = { if (entered.length < 6) { entered += it; error = null } },
            onDelete = { if (entered.isNotEmpty()) entered = entered.dropLast(1) },
            onSubmit = { submit() },
        )
        Spacer(Modifier.height(20.dp))
        Text(error ?: " ", fontSize = 13.sp, color = MaterialTheme.colorScheme.error, textAlign = TextAlign.Center)
        Spacer(Modifier.height(8.dp))
        if (mode == PrivacyLockMode.UNLOCK) {
            TextButton(onClick = { showForgot = true }) { Text("忘记密码？", fontSize = 13.sp) }
        }
        TextButton(onClick = onCancel) { Text("取消", fontSize = 14.sp) }
    }

    if (showForgot) {
        AlertDialog(
            onDismissRequest = { showForgot = false },
            title = { Text("忘记数字密码") },
            text = { Text("为保护私密内容，本应用不设密码找回，也不会一键重置放行。\n\n如需重置：系统设置 → 应用 → Kivo Cinema → 存储 → 清除数据。这会清空已保存的资源库与私密标记，需重新添加。", fontSize = 14.sp) },
            confirmButton = { TextButton(onClick = { showForgot = false }) { Text("我知道了") } },
        )
    }
}