package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.feedback.PlayerLevelIcon
import com.kivo.cinema.ui.compose.player.feedback.PlayerLevelMeter
import com.kivo.cinema.ui.compose.player.feedback.playerFeedbackSurface
import com.kivo.cinema.ui.motion.MotionGate
import kotlin.math.roundToInt

@Composable
internal fun PlayerGestureHud(
    visible: Boolean,
    kind: PlayerHudKind?,
    fraction: Float,
    modifier: Modifier = Modifier,
    originFraction: Float = fraction,
    dragging: Boolean = false,
) {
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val current = if (visible && (kind == PlayerHudKind.BRIGHTNESS || kind == PlayerHudKind.VOLUME)) {
        LevelFeedback(kind, fraction.coerceIn(0f, 1f), originFraction.coerceIn(0f, 1f))
    } else null
    var lastFeedback by remember { mutableStateOf<LevelFeedback?>(null) }
    SideEffect { if (current != null) lastFeedback = current }
    val visibility = remember { MutableTransitionState(false) }
    visibility.targetState = current != null
    val shown = current ?: lastFeedback ?: return
    val percentage = (shown.fraction * 100).roundToInt()
    val muted = shown.kind == PlayerHudKind.VOLUME && shown.fraction == 0f
    val name = when {
        shown.kind == PlayerHudKind.BRIGHTNESS -> "亮度"
        muted -> "静音"
        else -> "音量"
    }

    AnimatedVisibility(
        visibleState = visibility,
        enter = fadeIn(tween(PlayerMotion.hudEnterMs, easing = PlayerMotion.easing)),
        exit = fadeOut(tween(
            if (reduceMotion) PlayerMotion.reduceMotionMs else PlayerMotion.hudExitMs,
            easing = PlayerMotion.easing,
        )),
        modifier = modifier,
    ) {
        Column(
            modifier = Modifier
                .width(64.dp)
                .heightIn(min = 180.dp)
                .playerFeedbackSurface()
                .clearAndSetSemantics { contentDescription = "$name $percentage%" }
                .padding(horizontal = 8.dp, vertical = 12.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(4.dp, Alignment.CenterVertically),
        ) {
            PlayerLevelIcon(shown.kind, shown.fraction, Modifier.size(22.dp))
            Text(
                text = "$percentage%",
                color = Color.White,
                fontSize = 18.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
            )
            Text(
                text = name,
                color = Color.White.copy(alpha = 0.64f),
                fontSize = 10.sp,
                maxLines = 1,
            )
            key(shown.kind) {
                PlayerLevelMeter(
                    fraction = shown.fraction,
                    originFraction = shown.originFraction,
                    dragging = dragging && current != null,
                    modifier = Modifier.width(28.dp).height(80.dp),
                )
            }
        }
    }
}

private data class LevelFeedback(val kind: PlayerHudKind, val fraction: Float, val originFraction: Float)

internal enum class PlayerHudKind { BRIGHTNESS, VOLUME }
