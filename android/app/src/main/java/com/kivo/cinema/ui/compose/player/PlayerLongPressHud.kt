package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.feedback.playerFeedbackSurface
import com.kivo.cinema.ui.motion.MotionGate

@Composable
internal fun PlayerLongPressHud(
    speedLabel: String,
    locked: Boolean,
    modifier: Modifier = Modifier,
) {
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val active = speedLabel.isNotBlank()
    var lastLabel by remember { mutableStateOf("") }
    var lastLocked by remember { mutableStateOf(false) }
    SideEffect {
        if (active) {
            lastLabel = speedLabel
            lastLocked = locked
        }
    }
    val shownLabel = if (active) speedLabel else lastLabel
    val shownLocked = if (active) locked else lastLocked
    val visibility = remember { MutableTransitionState(false) }
    visibility.targetState = active
    val advance = remember { Animatable(0f) }
    LaunchedEffect(active) {
        if (active) {
            advance.snapTo(if (reduceMotion) 1f else 0f)
            if (!reduceMotion) advance.animateTo(
                1f, tween(PlayerMotion.longPressArrowMs, easing = PlayerMotion.easing),
            )
        }
    }
    val enterMs = if (reduceMotion) PlayerMotion.reduceMotionMs else PlayerMotion.longPressEnterMs
    val exitMs = if (reduceMotion) PlayerMotion.reduceMotionMs else PlayerMotion.longPressExitMs
    val enter = fadeIn(tween(enterMs, easing = PlayerMotion.easing))
    val exit = fadeOut(tween(exitMs, easing = PlayerMotion.easing))

    AnimatedVisibility(
        visibleState = visibility,
        enter = if (reduceMotion) enter else enter + scaleIn(
            tween(enterMs, easing = PlayerMotion.easing), initialScale = 0.96f,
        ),
        exit = if (reduceMotion) exit else exit + scaleOut(
            tween(exitMs, easing = PlayerMotion.easing), targetScale = 0.98f,
        ),
        modifier = modifier,
    ) {
        Row(
            modifier = Modifier
                .height(32.dp)
                .playerFeedbackSurface(cornerRadius = 16.dp)
                .padding(horizontal = 12.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            Canvas(Modifier.size(width = 24.dp, height = 16.dp)) {
                val shift = if (reduceMotion) 0f else (advance.value - 1f) * 5.dp.toPx()
                repeat(2) { index ->
                    val x = (7 + index * 9).dp.toPx() + shift
                    val arrow = Path().apply {
                        moveTo(x - 3.dp.toPx(), center.y - 4.dp.toPx())
                        lineTo(x + 1.dp.toPx(), center.y)
                        lineTo(x - 3.dp.toPx(), center.y + 4.dp.toPx())
                    }
                    drawPath(
                        arrow, Color.White.copy(alpha = 0.84f),
                        style = Stroke(1.7.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round),
                    )
                }
            }
            Text(
                text = shownLabel,
                color = Color.White,
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
            )
            if (shownLocked) {
                PlayerGlyph("lock", Color.White.copy(alpha = 0.8f), Modifier.size(12.dp))
            }
            Text(
                text = if (shownLocked) "锁定" else "倍速中",
                color = Color.White.copy(alpha = 0.72f),
                fontSize = 12.sp,
                maxLines = 1,
            )
        }
    }
}
