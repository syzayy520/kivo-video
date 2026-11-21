package com.kivo.cinema.ui.compose.player.row

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import com.kivo.cinema.ui.compose.player.panelPressable

/** 开关行：64 高；整行可点；题 16/22 + 副 12/17；行尾 [PlayerTogglePill]。 */
@Composable
internal fun PlayerToggleRow(
    item: PlayerOptionItem,
    onToggle: () -> Unit,
    modifier: Modifier = Modifier,
) {
    // 与 StepperHalf 同一坑：pointerInput 闭包冻结首帧 lambda，rememberUpdatedState 兜底。
    val currentOnToggle by rememberUpdatedState(onToggle)
    val tall = item.subtitle.isNotBlank()
    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.toggleRowHeight)
            .alpha(if (item.enabled) 1f else 0.38f)
            .panelPressable(enabled = item.enabled, onClick = currentOnToggle)
            .clearAndSetSemantics {
                contentDescription = item.title
                role = Role.Switch
                stateDescription = if (item.on) "已开启" else "已关闭"
                if (item.enabled) onClick { onToggle(); true }
            }
            .padding(end = PlayerPanelStyle.rowTrailingPad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = item.title,
                color = PlayerPanelStyle.rowTitleColor,
                fontSize = PlayerPanelStyle.rowTitleSize.sp,
                lineHeight = PlayerPanelStyle.rowTitleLineHeight.sp,
                letterSpacing = PlayerPanelStyle.rowTitleTracking.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (tall) {
                Text(
                    text = item.subtitle,
                    color = PlayerPanelStyle.rowSubtitleColor,
                    fontSize = PlayerPanelStyle.subtitleSize.sp,
                    lineHeight = PlayerPanelStyle.subtitleLineHeight.sp,
                    letterSpacing = PlayerPanelStyle.subtitleTracking.sp,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        PlayerTogglePill(on = item.on)
    }
}