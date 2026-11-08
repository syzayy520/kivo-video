package com.kivo.cinema.ui.design.home

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.spring
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.nestedscroll.NestedScrollConnection
import androidx.compose.ui.input.nestedscroll.NestedScrollSource
import androidx.compose.ui.unit.Velocity
import kotlinx.coroutines.launch

/** Owns only the unconsumed drag above the list's start. Units are physical px. */
internal class HeroPullZoom(
    val distance: Animatable<Float, androidx.compose.animation.core.AnimationVector1D>,
    val connection: NestedScrollConnection,
)

@Composable
internal fun rememberHeroPullZoom(atTop: () -> Boolean, maxPullPx: Float): HeroPullZoom {
    val pull = remember { Animatable(0f) }
    val scope = rememberCoroutineScope()
    val top = rememberUpdatedState(atTop)
    val limit = rememberUpdatedState(maxPullPx)
    return remember {
        val connection = object : NestedScrollConnection {
            override fun onPreScroll(available: Offset, source: NestedScrollSource): Offset {
                if (source != NestedScrollSource.UserInput || pull.value <= 0f || available.y >= 0f)
                    return Offset.Zero
                val used = available.y.coerceAtLeast(-pull.value / .5f)
                scope.launch { pull.snapTo((pull.value + used * .5f).coerceAtLeast(0f)) }
                return Offset(0f, used)
            }

            override fun onPostScroll(consumed: Offset, available: Offset, source: NestedScrollSource): Offset {
                if (source != NestedScrollSource.UserInput || available.y <= 0f || !top.value())
                    return Offset.Zero
                val resistance = .5f * (1f - pull.value / limit.value).coerceIn(.12f, 1f)
                scope.launch { pull.snapTo((pull.value + available.y * resistance).coerceAtMost(limit.value)) }
                return Offset(0f, available.y)
            }

            override suspend fun onPreFling(available: Velocity): Velocity {
                if (pull.value == 0f) return Velocity.Zero
                scope.launch { pull.animateTo(0f, spring(Spring.DampingRatioNoBouncy, Spring.StiffnessMediumLow)) }
                return Velocity(0f, available.y)
            }
        }
        HeroPullZoom(pull, connection)
    }
}
