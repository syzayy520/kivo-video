package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.net.HttpByteSource

/** URL 起播（Emby 等 HTTP 源）：镜像 PlayerSmbStart——预览挂在引擎分支前，token 只进头不进 URL。 */

internal fun PlayerComposeActivity.startUrlPlayback(holder: SurfaceHolder, url: String, label: String) {
    playbackStarted = true
    statusText = "正在准备网络播放…"
    val generation = ++networkGeneration
    val cancelled = java.util.concurrent.atomic.AtomicBoolean(false)
    networkPreparationCancelled = cancelled
    val token = intent.getStringExtra("kivo.embyToken")
    Thread({
        var source: HttpByteSource? = null
        var thumbSource: HttpByteSource? = null
        try {
            val opened = HttpByteSource(url, token)
            opened.size() // 探针：不支持 Range / 鉴权失败在这里就带 reason 退出
            source = opened
            // 预览第二读路径，与主读独立（同 SMB 口径）。打不开不致命：浮窗留形状+原因。
            thumbSource = runCatching { HttpByteSource(url, token) }.getOrNull()
            check(!cancelled.get())
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration || !surfaceReady) {
                    Thread {
                        runCatching { opened.close() }
                        runCatching { thumbSource?.close() }
                    }.start()
                    return@runOnUiThread
                }
                networkPreparationCancelled = null
                if (thumbSource != null) mountNetworkThumbnails(thumbSource)
                val engine = com.kivo.cinema.playback.PlaybackEngine.select()
                if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                    startMpvPlayback(holder, opened)
                    return@runOnUiThread
                }
                val range = com.kivo.cinema.net.RangeFd(
                    opened, getSystemService(android.os.storage.StorageManager::class.java),
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                )
                startNetworkPlayback(holder, com.kivo.cinema.playback.NetworkPlaybackSession(this, range), "emby")
            }
        } catch (e: Exception) {
            runCatching { source?.close() }
            runCatching { thumbSource?.close() }
            runOnUiThread {
                if (!cancelled.get() && !finishRequested && !isDestroyed && generation == networkGeneration) {
                    networkPreparationCancelled = null
                    statusText = e.message?.takeIf { it.isNotBlank() } ?: "网络连接失败（${e.javaClass.simpleName}）"
                    isPlaying = false
                    controlsVisible = true
                }
            }
        }
    }, "KivoUrlPrepare").start()
}