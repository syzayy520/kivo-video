package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.net.HttpOriginSource
import com.kivo.cinema.ui.compose.player.PlayerPhase

/**
 * URL 起播（Emby 等 HTTP 源）：**按连接隔离的预读取数**（273 v4）。
 *
 * 【v2 → v3】v2 用分片池：每 1 MiB 新建一条源站连接，读侧必须等整片到手。
 * 真机实测每次新建连接的表头延迟 0.4–2.4 s，而片源只消耗 415 KB/s；
 * 再叠加"没有超时、没有替补"，一条 28.9 KB/s 的慢连接就把读侧钉死 35.4 s
 * ⇒ mpv 侧 `Enter buffering` / `End buffering (waited 17.6 s)`。
 *
 * 【v3 → v4】v3 改成一条连接顺序读到底，卡顿从 11 次降到 5 次，
 * 但 t=350 s 仍连着卡 5 次（等待 14.5/10.3/6.3/4.4 s）。
 * 复盘：源站单连接能跑 4.9 MB/s（需求 415 KB/s），**瓶颈不在源站**，
 * 而在"按需取数"的节奏 —— mpv 要数据的瞬间才有供给，缓存被抽干。
 * v4 在取数层加**预读**：一条长连接持续把数据拉进环形缓冲，mpv 什么时候要都有。
 * 详见 [com.kivo.cinema.net.HttpOriginReader]。
 *
 * 【token】只进 `X-Emby-Token` 请求头，永不进 URL。
 */

internal fun PlayerComposeActivity.startUrlPlayback(holder: SurfaceHolder, url: String, label: String) {
    playbackStarted = true
    phase = PlayerPhase.Buffering
    statusText = "正在准备网络播放…"
    val generation = ++networkGeneration
    val cancelled = java.util.concurrent.atomic.AtomicBoolean(false)
    networkPreparationCancelled = cancelled
    val token = intent.getStringExtra("kivo.embyToken")
    // 273 v15：Emby 详情里就有片源总长（零额外请求）⇒ 片尾预取能在 t=0 发出去。
    val sizeHint = intent.getLongExtra("kivo.playSize", -1L)
    // 273 v15：主播放源在 Activity 起来时（onCreate）就建好了 ——
    // 源站握手与**应用自身启动**重叠，等 Surface 到了内核第一跳直接复用。
    val prep = urlPrep
    Thread({
        com.kivo.cinema.net.NetDiag.log("[prep] URL 起播开始")
        var owned: HttpOriginSource? = null
        var thumbRef: HttpOriginSource? = null
        try {
            val main = prep?.source ?: HttpOriginSource(
                url, token, "play",
                tailWindowBytes = tailWindowBytes(),
                seekWindowBytes = HttpOriginSource.DEFAULT_SEEK_WINDOW_BYTES,
                sizeHint = sizeHint,
            ).also {
                owned = it
                it.startWarmup()
            }
            // 内核在这里选（不进 UI 线程）：旧内核没有 loadFile 关口，
            // 只能在这个后台线程里等预备；新内核把等待挪进闸门（见下）。
            val engine = com.kivo.cinema.playback.PlaybackEngine.select()
            if (engine != com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                // 等预备就绪（有上限）：连接建好了，内核第一跳才可能**复用**，
                // 而不是自己新建一条去付 TLS 握手（真机 1.5 s vs 0.3 s）。
                prep?.awaitReady(PREP_READY_WAIT_MS)
                // 源站**明确拒绝**（401/403/404）才中止 —— 瞬时错误不判决，
                // 误杀一条本来能播的链路比多等 0.3 s 更糟。
                prep?.verdict?.let { throw it }
            }
            check(!cancelled.get())
            // 预览源**不预热**：预览大多数时候是关着的（scrubPreview=false），
            // 为一个不一定会用的实例白占两条源站连接不划算。预览自己慢一点可以忍，
            // 主播放的起播窗口才是要抢的那几秒。
            //
            // 【273 v26：预览与播放共用同一批窗口】预览源仍**独立**（自己的连接、
            // 自己的小块分块），但窗口缓存**借**播放主链路那份 ⇒ 落点窗口下载的
            // 那几 MB，预览与播放各取所需，**不必各下各的**（真机 v24：同一段
            // `from=1613145699` 被下了两次，带宽被劈成两半，两边都慢）。
            val thumb = HttpOriginSource(
                url, token, "thumb", THUMB_CAPACITY_BYTES, THUMB_CHUNK_BYTES, 0,
                tailWindowBytes = 0,
                externalWindowCache = main.windowCache,
            )
            thumbRef = thumb
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration || !surfaceReady) {
                    Thread {
                        runCatching { thumb.close() }
                        runCatching { owned?.close() }
                    }.start()
                    return@runOnUiThread
                }
                networkPreparationCancelled = null
                // 预览是**独立的源**：它的内核会另开连接去读，
                // 两个源共用一条流会互相把对方顶掉（长连接的流位置是单一游标）。
                // 但**窗口缓存是共用的**（见上）⇒ 落点那一段只下一次。
                mountNetworkThumbnails(thumb)
                if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                    // 273 v15：闸门只挡**第一跳**，内核创建与源站握手并行。
                    startMpvPlayback(holder, main, prep)
                    return@runOnUiThread
                }
                val range = com.kivo.cinema.net.RangeFd(
                    main, getSystemService(android.os.storage.StorageManager::class.java),
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                )
                startNetworkPlayback(holder, com.kivo.cinema.playback.NetworkPlaybackSession(this, range), "emby")
            }
        } catch (e: Exception) {
            runCatching { owned?.close() }
            runCatching { thumbRef?.close() }
            runOnUiThread {
                if (!cancelled.get() && !finishRequested && !isDestroyed && generation == networkGeneration) {
                    networkPreparationCancelled = null
                    statusText = e.message?.takeIf { it.isNotBlank() } ?: "网络连接失败（${e.javaClass.simpleName}）"
                    phase = PlayerPhase.Failed(statusText)
                    isPlaying = false
                    controlsVisible = true
                }
            }
        }
    }, "KivoUrlPrepare").start()
}

/**
 * 预览源的预读缓冲：2 MiB。
 *
 * 【为什么不是 4 MiB】**每个内核连接各有一份环**（内核每次 seek 都新开连接），
 * 而预读是**不看需求**的：环一满就把下一块也拉下来。
 * 预览一次会话要开三条连接（片头 / cues / seek 目标）⇒ 每条连接白下
 * "环 + 块"个字节。4 MiB 环 + 2 MiB 块 = 每连接 6 MiB ⇒ 三条就是 18 MiB。
 * 收成 2 MiB 环 + 1 MiB 块 = 每连接 3 MiB ⇒ 三条 9 MiB。
 * 真机实测（273 v12）预览一次会话总共下 32 MiB 只为显示一帧，
 * 环这块占了将近一半。
 *
 * 【不能更小】环太小会让每次拖动都重新建连（表头延迟 0.4–2.4 s），
 * 而拖动的首要指标是"浮窗多快冒出来"。
 */
private const val THUMB_CAPACITY_BYTES = 2 * 1024 * 1024

/**
 * 预览源的分块大小：1 MiB。
 *
 * 【为什么不是 256 KiB】预览的内核会**顺序读**（无索引 MKV 靠字节插值定位），
 * 按 256 KiB 分块就是很多次请求，每次都要付一次源站表头（真机 ≈300 ms）。
 *
 * 【为什么 1 MiB 不会更慢】首字节延迟只取决于**表头**，与块大小无关：
 * 块越大只是"同一次表头换来更多数据"。而 273 v12 之后每一跳都被代理
 * 截到 4 MiB（见 [com.kivo.cinema.mpv.LocalRangeResponder]），
 * 一跳最多 4 个块 ⇒ 最多多付 3 次表头（≈0.9 s），
 * 但换来的预读浪费从每连接 6 MiB 降到 3 MiB。
 */
private const val THUMB_CHUNK_BYTES = 1L * 1024 * 1024
