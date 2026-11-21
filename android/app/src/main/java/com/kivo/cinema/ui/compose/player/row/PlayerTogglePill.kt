package com.kivo.cinema.ui.compose.player.row

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.spring
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle

/** 开关 pill（46×28 / 圆角 14）：旋钮 240ms spring 位移；微调卡与列表共用。 */
@Composable
internal fun PlayerTogglePill(on: Boolean, modifier: Modifier = Modifier) {
    val travel = PlayerPanelStyle.toggleWidth - PlayerPanelStyle.toggleHeight
    val knobOffset by animateDpAsState(
        targetValue = if (on) travel else 0.dp,
        animationSpec = spring(dampingRatio = 0.85f, stiffness = 400f),
        label = "toggleKnob",
    )
    Box(
        modifier = modifier
            .width(PlayerPanelStyle.toggleWidth)
            .height(PlayerPanelStyle.toggleHeight)
            .clip(RoundedCornerShape(PlayerPanelStyle.toggleRadius))
            .background(if (on) PlayerPanelStyle.toggleTrackOn else PlayerPanelStyle.toggleTrackOff),
        contentAlignment = Alignment.CenterStart,
    ) {
        Box(
            modifier = Modifier
                .padding(start = 2.dp)
                .offset(x = knobOffset)
                .size(PlayerPanelStyle.toggleHeight - 4.dp)
                .clip(CircleShape)
                .background(if (on) PlayerPanelStyle.toggleKnobOn else PlayerPanelStyle.toggleKnobOff),
        )
    }
}