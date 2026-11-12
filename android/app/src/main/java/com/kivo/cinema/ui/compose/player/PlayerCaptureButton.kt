package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp

/**
 * 暂停态左中浮相机钮（v3 §1.5，iPhone 现屏实测：48pt 毛玻璃圆、中心 ≈(88,195)pt）。
 *
 * 只在**暂停且控制条显形**时出现（播放态无此钮）；点＝截当前帧。
 * 与拖动预览里的相机钮共用 [PlayerActionRouter.onCapture]，不另写截图逻辑。
 */
@Composable
internal fun PlayerCaptureButton(
    onCapture: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(
        modifier = modifier
            .playerSafeArea(WindowInsetsSides.Horizontal)
            .padding(start = PlayerTokens.captureFloatStart.dp)
            .size(PlayerTokens.captureFloatSize.dp)
            .background(PlayerTokens.panelSurface, CircleShape)
            .semantics { contentDescription = "截取当前帧" }
            .pointerInput(Unit) { detectTapGestures(onTap = { onCapture() }) },
        contentAlignment = Alignment.Center,
    ) {
        PlayerGlyph(
            kind = "camera",
            color = PlayerTokens.textPrimary,
            modifier = Modifier.size(PlayerTokens.captureFloatGlyph.dp),
        )
    }
}