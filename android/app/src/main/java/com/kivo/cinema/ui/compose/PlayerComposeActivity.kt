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
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.layout.padding
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
class PlayerComposeActivity : ComponentActivity() {

    private var playbackSession: PlaybackSession? = null
    private var surfaceReady = false
    private var playbackStarted = false

    private var mediaOverridePath = ""
    private var evidenceName = ""
    private var thumbnailReader: com.kivo.cinema.thumbnail.ThumbnailReader? = null
    private var stressDone = false

    // Compose 状态（D4：控制层 + 播放状态）
    private var isPlaying by mutableStateOf(false)
    private var controlsVisible by mutableStateOf(true)
    private var positionMs by mutableStateOf(0L)
    private var durationMs by mutableStateOf(0L)
    private var statusText by mutableStateOf("starting...")
    private var thumbnailBitmap by mutableStateOf<android.graphics.Bitmap?>(null)
    private var previewMs by mutableStateOf(-1L)

    private val positionPoll = object : Runnable {
        override fun run() {
            val session = playbackSession ?: return
            // EOS 后时钟外推会越过片长（真机实测 205s/60s 片）——clamp 到时长
            positionMs = (session.positionUs() / 1000).coerceAtMost(durationMs)
            // 3s 无操作自动隐藏（D4）；预览窗口期间不隐藏（取证 harness 需要）
            if (previewMs < 0 && System.currentTimeMillis() - lastTouchMs > 3000) {
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
                    // 诊断状态（M1 临时：K40 logcat 不可用的实时替代）
                    androidx.compose.material3.Text(
                        text = statusText,
                        color = Color.White,
                        fontSize = androidx.compose.ui.unit.TextUnit(10f, androidx.compose.ui.unit.TextUnitType.Sp),
                        modifier = Modifier.align(androidx.compose.ui.Alignment.TopStart)
                            .background(Color.Black.copy(alpha = 0.7f))
                            .padding(4.dp)
                    )
                    if (controlsVisible) {
                        PlayerControls(
                            isPlaying = isPlaying,
                            positionMs = positionMs,
                            durationMs = durationMs,
                            onPlayPause = { togglePlayPause() },
                            onSeek = { ms -> playbackSession?.seekTo(ms * 1000) },
                            onScrub = { ms ->
                                lastTouchMs = System.currentTimeMillis()   // 拖动也算触摸，防 3s 自动隐藏打断长拖
                                thumbnailReader?.seekTo(ms * 1000)
                            },
                            thumbnailBitmap = thumbnailBitmap,
                            previewMs = previewMs,
                            modifier = Modifier.align(androidx.compose.ui.Alignment.BottomCenter),
                        )
                    }
                }
            }
        }
    }

    // PlayerSurfaceView.Listener 的回调（由 AndroidView 工厂里的匿名 Listener 转发）
    fun onSurfaceAvailable(holder: SurfaceHolder) {
        surfaceReady = true
        maybeStartPlayback(holder)
    }

    fun onSurfaceLost() {
        surfaceReady = false
    }

    private fun maybeStartPlayback(holder: SurfaceHolder) {
        // surface 回调本身就是"面可用"的证明——不再依赖 Listener 的 surfaceReady 标志
        surfaceReady = true
        if (playbackStarted) return
        val media = mediaOverridePath.takeIf { it.isNotBlank() }
            ?.let { java.io.File(it) }?.takeIf { it.isFile }
            ?: locateTestMedia() ?: return
        playbackStarted = true

        statusText = "surface ok, probing..."
        val routing = FfmpegRouting.probe(media)
        statusText = "route=${routing.decision} reason=${routing.reason}"
        Log.i(TAG, "routing: ${routing.decision} (${routing.reason})")

        val session = PlaybackSession(0, media, media.name)
        playbackSession = session

        statusText = "openVideo..."
        val rc = session.openVideo(holder.surface)
        statusText = "openVideo rc=$rc lastError=${session.lastError}"
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

        statusText = "starting..."
        val startRc = session.start()
        statusText = "start rc=$startRc"
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
        // 时长：一次性 extractor 探测（215 拖动预览需要；顺带修掉控制条时长恒 0:00:00 的 213 缺口）
        durationMs = probeDurationUs(media.absolutePath) / 1000
        statusText = "playing ${media.name}"

        // 215：独立缩略图读取器（懒初始化；与主会话完全解耦）
        val reader = com.kivo.cinema.thumbnail.ThumbnailReader(media.absolutePath)
        thumbnailReader = reader
        reader.onThumbnailReady = { _, bitmap ->
            android.os.Handler(mainLooper).post { thumbnailBitmap = bitmap }
        }

        // 215 取证 harness：`kivo.evidence=p215` 时自动跑 60 点扫描 + 主链路对照
        if (evidenceName == "p215") scheduleP215Stress()

        // 214：会话所有权转移给 Foreground Service——Activity 销毁后会话不死。
        com.kivo.cinema.playback.PlaybackSessionHolder.offer(session)
        startForegroundService(
            android.content.Intent(this, com.kivo.cinema.playback.PlaybackService::class.java)
        )
        Log.i(TAG, "playback started: ${media.absolutePath} (service adopted)")
    }

    private fun togglePlayPause() {
        val session = playbackSession ?: return
        isPlaying = !isPlaying
        if (isPlaying) session.resume() else session.pause()
    }

    // ---- 215 取证 harness（`kivo.evidence=p215`）：60 点扫描 + 主链路 writeStats 对照 ----
    private var stressDurationUs = -1L

    private fun scheduleP215Stress() {
        android.os.Handler(mainLooper).postDelayed({ runP215Stress(0) }, 2000)
    }

    /** 片长：harness 自己开一次性 extractor 读 KEY_DURATION。
     *  不经 [thumbnailReader]——reader 的第一次使用必须留给第一个 seekTo（AC1 冷启动含 init 才诚实）。 */
    private fun probeDurationUs(path: String): Long {
        return try {
            val ex = android.media.MediaExtractor()
            ex.setDataSource(path)
            var dur = -1L
            for (i in 0 until ex.trackCount) {
                val f = ex.getTrackFormat(i)
                if (f.getString(android.media.MediaFormat.KEY_MIME)?.startsWith("video/") == true) {
                    dur = f.getLong(android.media.MediaFormat.KEY_DURATION)
                    break
                }
            }
            ex.release()
            dur
        } catch (e: Exception) {
            -1L
        }
    }

    private fun runP215Stress(index: Int) {
        val session = playbackSession ?: return
        val reader = thumbnailReader ?: return
        val evidenceDir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        if (index == 0) {
            statusText = "p215 baseline writeStats..."
            session.writeStats("${evidenceDir.absolutePath}/p215-stats-baseline.json")
            stressDurationUs = probeDurationUs(currentMediaPath())
        }
        if (index >= 60) {
            // 扫描完：补一次已缓存位置（cache_hit 通道），然后收证据
            statusText = "p215 scan done, cache-hit probe..."
            reader.seekTo(stressDurationUs / 4)   // 60 点覆盖全片，1/4 处必已缓存
            android.os.Handler(mainLooper).postDelayed({ finishP215Stress(evidenceDir) }, 1500)
            return
        }
        if (index == 30 && evidenceName == "p215") {
            // 预览演示窗口：与拖动共用同一条浮窗 UI（3s，给截图留确定性窗口）
            previewMs = (stressDurationUs / 2) / 1000
            controlsVisible = true
            android.os.Handler(mainLooper).postDelayed({ previewMs = -1L }, 4000)
        }
        if (stressDurationUs > 0) {
            val posUs = ((index + 0.5) / 60.0 * stressDurationUs).toLong()
            reader.seekTo(posUs)
        }
        statusText = "p215 scan ${index + 1}/60"
        android.os.Handler(mainLooper).postDelayed({ runP215Stress(index + 1) }, 250)
    }

    private fun currentMediaPath(): String {
        mediaOverridePath.takeIf { it.isNotBlank() }?.let { return it }
        return locateTestMedia()?.absolutePath ?: ""
    }

    private fun finishP215Stress(evidenceDir: java.io.File) {
        val session = playbackSession ?: return
        val reader = thumbnailReader ?: return
        session.writeStats("${evidenceDir.absolutePath}/p215-stats-stress.json")
        val thumb = org.json.JSONObject(reader.evidenceJson())
        val meta = org.json.JSONObject()
        meta.put("device_model", android.os.Build.MODEL)
        meta.put("device_product", android.os.Build.PRODUCT)
        meta.put("media", mediaOverridePath.ifBlank { "located-test-media" })
        meta.put("stress_points", 60)
        meta.put("scan_spacing_ms", 250)
        val root = org.json.JSONObject()
        root.put("schema", "kivo.p215.thumbnail/1")
        root.put("generated_at_unix_ms", System.currentTimeMillis())
        root.put("meta", meta)
        root.put("thumbnail", thumb)
        val out = java.io.File(evidenceDir, "p215-thumb.json")
        out.writeText(root.toString(2))
        stressDone = true
        statusText = "p215 evidence written: ${out.name}"
        Log.i(TAG, "p215 evidence: $out")
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
        thumbnailReader?.release()
        thumbnailReader = null
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}
