package com.kivo.cinema.sources.ui.workspace

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp

/** Two full-height pages. Only the primary page owns application navigation. */
@Composable
internal fun SourceWorkspace(open: Boolean, navigation: @Composable () -> Unit,
    overview: @Composable () -> Unit, panel: @Composable (Boolean) -> Unit) {
    BoxWithConstraints(Modifier.fillMaxSize()) {
        val split = maxWidth >= 960.dp && LocalDensity.current.fontScale <= 1.3f && open
        Row(Modifier.fillMaxSize()) {
            Box(Modifier.weight(1f).fillMaxHeight()) {
                Box(Modifier.fillMaxSize().statusBarsPadding().navigationBarsPadding().padding(bottom = 94.dp)) {
                    overview()
                }
                Box(Modifier.align(Alignment.BottomCenter).navigationBarsPadding().padding(bottom = 22.dp)) {
                    navigation()
                }
            }
            if (split) {
                var entered by remember { mutableStateOf(false) }
                LaunchedEffect(Unit) { entered = true }
                val reveal by animateFloatAsState(if (entered) 1f else 0f, tween(220), label = "source page reveal")
                // Both pages inherit one ambient field; a subtle wash defines the secondary page.
                Box(Modifier.weight(1f).fillMaxHeight().background(Color.White.copy(alpha = .24f))) {
                    Box(Modifier.width(.5.dp).fillMaxHeight()
                        .background(MaterialTheme.colorScheme.onSurface.copy(alpha = .08f)))
                    Box(Modifier.fillMaxSize().statusBarsPadding().navigationBarsPadding().imePadding()
                        .graphicsLayer { alpha = reveal; translationX = (1f - reveal) * 12.dp.toPx() }) {
                        panel(true)
                    }
                }
            }
        }
        if (!split && open) panel(false)
    }
}
