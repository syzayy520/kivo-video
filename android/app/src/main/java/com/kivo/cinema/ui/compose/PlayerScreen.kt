package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.viewinterop.AndroidView
import com.kivo.cinema.ui.PlayerSurfaceView

/**
 * 画面层（213 设计稿 D2）：`AndroidView` 包既有 `PlayerSurfaceView`——
 * 零拷贝路径一行不动。
 *
 * **这里只有画面，没有手势**：手势层搬到了
 * [com.kivo.cinema.ui.compose.player.PlayerGestureLayer]，
 * 由播放页的覆盖层统一管理。
 * 原因见那个文件的说明：手势层若留在画面层里，会把顶栏/底栏的点击**全部吃掉**
 * （第一版实机：按钮全部点不动）。
 */
@Composable
fun PlayerScreen(
    onSurfaceReady: (SurfaceHolder) -> Unit,
    onSurfaceLost: () -> Unit,
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
    }
}
