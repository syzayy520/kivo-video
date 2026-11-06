package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.viewinterop.AndroidView
import com.kivo.cinema.ui.PlayerSurfaceView

/**
 * 画面层（213 设计稿 D2）：`AndroidView` 包既有 `PlayerSurfaceView`——
 * 零拷贝路径一行不动。手势：单击切控制条、双击切播放/暂停。
 */
@Composable
fun PlayerScreen(
    onSurfaceReady: (SurfaceHolder) -> Unit,
    onSurfaceLost: () -> Unit,
    onTap: () -> Unit,
    onDoubleTap: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier.fillMaxSize()) {
        AndroidView(
            factory = { context ->
                PlayerSurfaceView(context).apply {
                    listener = object : PlayerSurfaceView.Listener {
                        override fun onSurfaceAvailable(holder: SurfaceHolder) {
                            onSurfaceReady(holder)
                        }
                        override fun onSurfaceLost() {
                            onSurfaceLost()
                        }
                    }
                }
            },
            modifier = Modifier.fillMaxSize(),
        )
        // 手势层浮在 SurfaceView 上（只消费 tap，不遮挡画面渲染）
        Box(
            modifier = Modifier
                .fillMaxSize()
                .pointerInput(Unit) {
                    detectTapGestures(
                        onTap = { onTap() },
                        onDoubleTap = { onDoubleTap() },
                    )
                },
        )
    }
}
