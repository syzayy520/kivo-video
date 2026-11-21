package com.kivo.cinema.ui.compose.player.row

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import com.kivo.cinema.ui.compose.player.panelPressable

/** 重置行：48 高、14sp/68% 居中（固定在卡底，保留 FAIL-2 修复）。 */
@Composable
internal fun PlayerResetRow(onReset: () -> Unit, modifier: Modifier = Modifier) {
    val currentOnReset by rememberUpdatedState(onReset)
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.resetRowHeight)
            .panelPressable(enabled = true, scaleOnPress = false, onClick = currentOnReset)
            .clearAndSetSemantics {
                contentDescription = "重置"
                role = Role.Button
                onClick { currentOnReset(); true }
            },
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = "重置",
            color = PlayerPanelStyle.resetColor,
            fontSize = PlayerPanelStyle.resetSize.sp,
            lineHeight = PlayerPanelStyle.resetLineHeight.sp,
            letterSpacing = 0.1.sp,
            textAlign = TextAlign.Center,
        )
    }
}