package com.kivo.cinema.ui.compose

import android.media.AudioManager
import android.os.Bundle
import android.view.SurfaceHolder
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.PlayerSurfaceView

/**
 * Compose 播放页宿主（213）。
 *
 * 与旧 [com.kivo.cinema.ui.PlayerActivity] 并存灰度（设计稿 D5：`kivo.ui=compose`）。
 * 会话生命周期复用 [PlaybackSession]——native 桥一行不动。
 * 视频层走 AndroidView(PlayerSurfaceView)——零拷贝路径一行不动（设计稿 D2）。
 *
 * M1 明确不做：字幕叠加的 Compose 集成（Android 侧 SubtitleOverlayView 仍可用，
 * Compose 集成后续卡）、音频分离控制、横竖屏切换。
 */
class PlayerComposeActivity : ComponentActivity(), PlayerSurfaceView.Listener {

    private lateinit var surfaceView: PlayerSurfaceView
    private var playbackSession: PlaybackSession? = null
    private var surfaceReady = false
    private var playbackStarted = false

    private var mediaOverridePath = ""
    private var evidenceName = ""

    // Compose 状态（D4：控制层 + 播放状态）
    private var isPlaying by mutableStateOf(false)
    private var controlsVisible by mutableStateOf(true)
    private var positionMs by mutableStateOf(0L)
    private var durationMs by mutableStateOf(0L)

    private val positionPoll = object : Runnable {
        override fun run() {
            val session = playbackSession ?: return
            positionMs = session.positionUs() / 1000
            // 3s 无操作自动隐藏（D4）
            if (System.currentTimeMillis() - lastTouchMs > 3000) {
                controlsVisible = false
            }
            android.os.Handler(mainLooper).postDelayed(this, 100)
        }
    }
    private var lastTouchMs = 0L

    companion object {
        private const val TAG = "KivoCompose"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        readProbeExtras()

        surfaceView = PlayerSurfaceView(this).apply { listener = this@PlayerComposeActivity }

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
                    PlayerScreen(
                        onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                        onSurfaceLost = { surfaceReady = false },
                        onTap = {
                            lastTouchMs = System.currentTimeMillis()
                            controlsVisible = !controlsVisible
                        },
                        onDoubleTap = {
                            lastTouchMs = System.currentTimeMillis()
                            togglePlayPause()
                        },
                    )
                    if (controlsVisible) {
                        PlayerControls(
                            isPlaying = isPlaying,
                            positionMs = positionMs,
                            durationMs = durationMs,
                            onPlayPause = { togglePlayPause() },
                            onSeek = { ms -> playbackSession?.seekTo(ms * 1000) },
                            modifier = Modifier.align(androidx.compose.ui.Alignment.BottomCenter),
                        )
                    }
                }
            }
        }
    }

    // PlayerSurfaceView.Listener（AndroidView 内部的 Surface 回调）
    override fun onSurfaceAvailable(holder: SurfaceHolder) {
        surfaceReady = true
        maybeStartPlayback(holder)
    }

    override fun onSurfaceLost() {
        surfaceReady = false
    }

    private fun maybeStartPlayback(holder: SurfaceHolder) {
        if (!surfaceReady || playbackStarted) return
        val media = mediaOverridePath.takeIf { it.isNotBlank() }
            ?.let { java.io.File(it) }?.takeIf { it.isFile }
            ?: locateTestMedia() ?: return
        playbackStarted = true

        val routing = FfmpegRouting.probe(media)
        Log.i(TAG, "routing: ${routing.decision} (${routing.reason})")

        val session = PlaybackSession(0, media, media.name)
        playbackSession = session

        val rc = session.openVideo(holder.surface)
        if (rc != 0) {
            Log.e(TAG, "openVideo failed rc=$rc: ${session.lastError}")
            session.close()
            playbackSession = null
            finish()
            return
        }

        // 节拍（字幕 position_us 的时间源；不加节拍时 position_us 恒 -1）
        session.configurePacing(targetFps = 0, allowDrop = true)
        session.configurePipeline(split = false, present = true, atTime = true)

        val startRc = session.start()
        if (startRc != 0) {
            Log.e(TAG, "start failed rc=$startRc")
            session.close()
            playbackSession = null
            finish()
            return
        }
        isPlaying = true
        lastTouchMs = System.currentTimeMillis()
        android.os.Handler(mainLooper).post(positionPoll)
        Log.i(TAG, "playback started: ${media.absolutePath}")
    }

    private fun togglePlayPause() {
        val session = playbackSession ?: return
        isPlaying = !isPlaying
        if (isPlaying) session.resume() else session.pause()
    }

    private fun readProbeExtras() {
        mediaOverridePath = intent.getStringExtra("kivo.media") ?: ""
        evidenceName = intent.getStringExtra("kivo.evidence") ?: ""
    }

    private fun locateTestMedia(): java.io.File? {
        val dir = java.io.File(getExternalFilesDir(null), "media")
        return dir.listFiles()?.filter { it.isFile }?.minByOrNull { it.name }
    }

    override fun onDestroy() {
        android.os.Handler(mainLooper).removeCallbacks(positionPoll)
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}
