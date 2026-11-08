package com.kivo.cinema.ui.detail.loading

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Snackbar
import androidx.compose.material3.SnackbarDuration
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.SnackbarResult
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens

/** Error presentation only; loading and retry remain owned by the detail page. */
@Composable
internal fun DetailLoadFeedback(
    error: String?, hasContent: Boolean, loading: Boolean,
    onRetry: () -> Unit, modifier: Modifier = Modifier,
) {
    val host = remember { SnackbarHostState() }
    LaunchedEffect(error, hasContent) {
        host.currentSnackbarData?.dismiss()
        if (error != null && hasContent) {
            val result = host.showSnackbar(
                message = "$error，已保留当前内容",
                actionLabel = "重试", withDismissAction = true,
                duration = SnackbarDuration.Indefinite,
            )
            if (result == SnackbarResult.ActionPerformed) onRetry()
        }
    }
    if (hasContent) {
        SnackbarHost(host, modifier) { data ->
            Snackbar(
                snackbarData = data,
                shape = RoundedCornerShape(20.dp),
                containerColor = DesignTokens.pageBackground,
                contentColor = DesignTokens.textMain,
                actionColor = DesignTokens.textMain,
                dismissActionContentColor = DesignTokens.textSub,
            )
        }
    } else if (error != null) {
        Surface(modifier, shape = RoundedCornerShape(24.dp),
            color = DesignTokens.pageBackground) {
            Column(Modifier.padding(24.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                Text(error, color = DesignTokens.textMain)
                TextButton(onClick = onRetry, enabled = !loading) {
                    Text(if (loading) "正在重试…" else "重试加载", color = DesignTokens.textMain)
                }
            }
        }
    }
}
