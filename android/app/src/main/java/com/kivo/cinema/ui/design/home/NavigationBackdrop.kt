package com.kivo.cinema.ui.design.home

import androidx.compose.foundation.layout.Box
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.BlurEffect
import androidx.compose.ui.graphics.TileMode
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.layer.GraphicsLayer
import androidx.compose.ui.graphics.layer.drawLayer
import androidx.compose.ui.graphics.rememberGraphicsLayer
import androidx.compose.ui.graphics.drawscope.translate
import androidx.compose.ui.graphics.drawscope.clipRect
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.layout.positionInRoot

/** Only explicitly registered Compose sources can enter the glass capture. */
class NavigationBackdropState {
    internal val sources = mutableStateListOf<NavigationSource>()
}

internal class NavigationSource(val layer: GraphicsLayer) {
    var origin by mutableStateOf(Offset.Zero)
}

@Composable
internal fun Modifier.navigationSource(state: NavigationBackdropState?, enabled: Boolean = true): Modifier {
    val layer = rememberGraphicsLayer()
    val source = remember(layer) { NavigationSource(layer) }
    DisposableEffect(state, enabled) {
        if (enabled) state?.sources?.add(source)
        onDispose { state?.sources?.remove(source) }
    }
    if (!enabled || state == null) return this
    return onGloballyPositioned { source.origin = it.positionInRoot() }
        .drawWithContent {
            layer.record { this@drawWithContent.drawContent() }
            drawLayer(layer)
        }
}

/** Native WebView/video render nodes are never registered or replayed. */
@Composable
internal fun NavigationBackdrop(state: NavigationBackdropState, modifier: Modifier = Modifier) {
    var origin by remember { mutableStateOf(Offset.Zero) }
    Box(modifier.onGloballyPositioned { origin = it.positionInRoot() }
        .graphicsLayer { renderEffect = BlurEffect(24f, 24f, TileMode.Clamp) }
        .drawWithContent {
            clipRect {
                state.sources.forEach { source ->
                    translate(source.origin.x - origin.x, source.origin.y - origin.y) {
                        drawLayer(source.layer)
                    }
                }
            }
        })
}
