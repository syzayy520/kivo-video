package com.kivo.cinema.ui.detail

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 危险操作确认弹窗（272 审计 D4）。
 *
 * 「移出待看」是不可逆的列表删除，点一下就消失 ⇒ 必须先问一句。
 * 形态沿用设置页清缓存的确认弹窗（同一套语言：白底、无阴影、取消在左、危险色在右）。
 */
@Composable
internal fun DetailConfirmDialog(
    title: String,
    message: String,
    confirmLabel: String,
    onConfirm: () -> Unit,
    onDismiss: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title, fontSize = 17.sp) },
        text = { Text(message, fontSize = 14.sp, lineHeight = 20.sp) },
        confirmButton = {
            Text(confirmLabel, color = DesignTokens.dangerRed, fontSize = 15.sp,
                modifier = Modifier.clickable(role = Role.Button) {
                    onConfirm(); onDismiss()
                }.padding(8.dp))
        },
        dismissButton = {
            Text("取消", color = DesignTokens.textSub, fontSize = 15.sp,
                modifier = Modifier.clickable(role = Role.Button) { onDismiss() }.padding(8.dp))
        },
    )
}
