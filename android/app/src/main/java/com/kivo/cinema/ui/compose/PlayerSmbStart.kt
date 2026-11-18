package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.playback.OriginSupplyPressure
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import com.kivo.cinema.ui.compose.player.PlayerPhase

/** SMB 起播：mpv / 硬解 / 软解三链路互斥；软解链路见 PlayerSmbSoftStart.kt。 */

internal fun PlayerComposeActivity.startSmbPlayback(
    holder: SurfaceHolder, sourceId: String, filePath: String, shareOverride: String = "",
) {
    playbackStarted = true
    phase = PlayerPhase.Buffering
    statusText = "正在连接SMB…"
    val generation = ++networkGeneration
    val cancelled = java.util.concurrent.atomic.AtomicBoolean(false)
    networkPreparationCancelled = cancelled
    Thread({
        var source: SmbByteSource? = null
        var thumbSource: SmbByteSource? = null
        try {
            val record = com.kivo.cinema.sources.storage.SourceStore(this)
                .read().getOrNull()?.firstOrNull { it.id == sourceId }
                ?: throw IllegalStateException("找不到这个资源库，可能已被移除")
            val parsed = SmbAddress.parse(record.address)
            val address = if (shareOverride.isNotBlank()) parsed.copy(share = shareOverride) else parsed
            check(!address.needsShare) { "这个资源库还没有选择共享" }
            val opened = SmbByteSource.open(
                address, SmbCredential(username = record.username, password = record.password), filePath,
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                // MIUI 过滤应用 logcat：读画像必须落盘才看得到（252 实测 50s 只剩 74 行）。
                reportSink = { line -> smbTrace("smbread $line") },
            )
            source = opened
            smbTrace("prepared open size=${opened.size()}")
            // v3.1：缩略图第二读路径（随机读、预读关）。与主读共用会互顶预读窗口（252）。
            // 打不开不致命：预览浮窗保持形状、内容留空，取证记 decode_failures。
            thumbSource = runCatching {
                SmbByteSource.open(
                    address, SmbCredential(username = record.username, password = record.password), filePath,
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                )
            }.getOrNull()
            // v3.1c：seek 后 demux 是顺序读簇，预读把 8MiB 回填从几十次 SMB
            // 往返（真机 1.0–2.9s/帧）压成一次；本源独立于主读，不互顶窗口。
            thumbSource?.setPrefetchEnabled(true)
            check(!cancelled.get())
            val range = com.kivo.cinema.net.RangeFd(
                opened, getSystemService(android.os.storage.StorageManager::class.java),
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
            )
            // 网络片源不做容器探测（252）：Shelter 12GB 无索引，线性扫全片 13 分钟不返回。
            // 先按平台解码走，解不了第一帧再退软解。
            val routing = com.kivo.cinema.bridge.FfmpegRouting.Decision(
                useFfmpeg = false, decision = "mediacodec",
                reason = "网络片源跳过容器探测（252）；平台解不了再退软解",
            )
            smbTrace("routing=${routing.decision} reason=${routing.reason}")
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration || !surfaceReady) {
                    Thread {
                        runCatching { range.close() }
                        runCatching { opened.close() }
                        runCatching { thumbSource?.close() }
                    }.start()
                    return@runOnUiThread
                }
                networkPreparationCancelled = null
                // 缩略图与弹幕和内核引擎无关，必须在引擎分支**之前**挂：
                // mpv 分支会 return，放在分支后等于日常路径永远不挂（真机实测）。
                if (thumbSource != null) mountNetworkThumbnails(thumbSource)
                loadDanmaku(filePath)
                val engine = com.kivo.cinema.playback.PlaybackEngine.select()
                if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                    smbTrace("engine=mpv")
                    startMpvPlayback(holder, opened)
                    return@runOnUiThread
                }
                if (routing.useFfmpeg) {
                    opened.setPrefetchEnabled(true)   // 软解顺序流式读，预读命中实测 100%
                    startSoftPlayback(holder, range, routing, filePath, generation, cancelled,
                        address, SmbCredential(username = record.username, password = record.password))
                } else {
                    // 硬解是突发读：预读会跟主读抢 SMB 链路（实测 24→20.27fps），关掉。
                    opened.setPrefetchEnabled(false)
                    val net = com.kivo.cinema.playback.NetworkPlaybackSession(this, range)
                    startNetworkPlayback(holder, net, "smb") {
                        // 平台解不了（VC-1）→ 重开源走软解。不能复用原 range：会话关闭会连 source 一起关。
                        smbTrace("platform decode failed (${net.lastError}) -> ffmpeg (rebind)")
                        runCatching { net.stop() }
                        val credentials = SmbCredential(record.username, record.password)
                        Thread {
                            try {
                                val fresh = SmbByteSource.open(address, credentials, filePath,
                                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
                                fresh.setPrefetchEnabled(true)
                                val freshRange = com.kivo.cinema.net.RangeFd(
                                    fresh, getSystemService(android.os.storage.StorageManager::class.java),
                                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
                                runOnUiThread {
                                    if (cancelled.get() || finishRequested || isDestroyed) {
                                        runCatching { freshRange.close() }
                                        return@runOnUiThread
                                    }
                                    startSoftPlayback(holder, freshRange,
                                        com.kivo.cinema.bridge.FfmpegRouting.Decision(
                                            true, "ffmpeg", "platform decode failed; fallback (252)"),
                                        filePath, generation, cancelled, address, credentials)
                                }
                            } catch (e: Exception) {
                                runOnUiThread {
                                    statusText = "这个片源无法播放：${e.message?.take(80)}"
                                    phase = PlayerPhase.Failed("这个片源无法播放：${e.message?.take(80) ?: "未知错误"}")
                                    controlsVisible = true
                                }
                            }
                        }.start()
                    }
                }
            }
        } catch (e: Exception) {
            runCatching { source?.close() }
            runCatching { thumbSource?.close() }
            runOnUiThread {
                if (!cancelled.get() && !finishRequested && !isDestroyed && generation == networkGeneration) {
                    networkPreparationCancelled = null
                    statusText = e.message?.takeIf { it.isNotBlank() } ?: "SMB连接失败（${e.javaClass.simpleName}）"
                    phase = PlayerPhase.Failed(statusText)
                    isPlaying = false
                    controlsVisible = true
                }
            }
        }
    }, "KivoSmbPrepare").start()
}

/**
 * mpv 内核起播：吃 ByteSource（SMB / HTTP / 本地文件，代理按 offset 读），与 fd 链路互斥。
 *
 * 【注意】Kotlin 的块注释**可以嵌套**：注释正文里若出现"斜杠 + 双星号"（哪怕只是
 * 路径或加粗排版的一部分），会被解析成新开一层注释 ⇒ 后面的闭合符只关内层，
 * 外层一直不闭合，报 `Syntax error: Unclosed comment`。这里栽过一次，别再写。
 */
internal fun PlayerComposeActivity.startMpvPlayback(
    holder: SurfaceHolder,
    /** 需要代理的源。354 起可为 null —— 本地文件直连时不需要它（mpv 自己读文件）。 */
    source: com.kivo.cinema.net.ByteSource? = null,
    /** 起播放行闸门（null = 不设）：URL 起播用它挡第一跳，SMB/本地不用。 */
    gate: com.kivo.cinema.playback.PlaybackLoadGate? = null,
    /**
     * 是否是**本地文件**（280）。只影响 [smbPlayback] 标记 ——
     * 它被投屏桥读作"时长未知"（-2）。本地文件有确定时长，不该被当成网络流。
     */
    isLocal: Boolean = false,
    /** 354 · 本地文件绝对路径。非空即有资格直连（`loadfile <path>`），此时不需要 [source]。 */
    localPath: String? = null,
) {
    if (finishRequested || isDestroyed) return
    playbackStarted = true
    smbPlayback = !isLocal
    // 280 · 指标：**不在这里 begin**（会被 PlaybackMetrics 的入口守卫拦住，也不该拦）。
    // 起点统一由 maybeStartPlayback 打 —— 那里比本函数早，且覆盖全部路径。
    phase = PlayerPhase.Buffering
    statusText = "正在准备内核播放…"
    val session = com.kivo.cinema.playback.MpvPlaybackSession(this, source, localPath)
    mpvNetSession = session
    // 续播位在**起播前**取：交给内核的每文件选项 `start=`，
    // 内核打开即落到该位置（否则要等首帧出来再补一条 seek，白等 0.5–0.8 秒）。
    val resumeMs = resumePositionMs()
    session.start(holder.surface, { result ->
        if (finishRequested || isDestroyed || mpvNetSession !== session) return@start
        if (result == 0) {
            // 280 · 指标：首帧就绪 = 用户真正看到画面的时刻（275 §6.1 的 startup_total 终点）。
            com.kivo.cinema.playback.PlaybackMetrics.mark(
                com.kivo.cinema.playback.PlaybackMetrics.T_FIRST_FRAME
            )
            // 取证锚点：首帧就绪的**绝对时刻**。
            // 内核日志里的时间是相对 mpv 进程时钟的，与主机侧的 tap 时刻对不上；
            // 没有这个锚点，"从点播放到出画面"就只能靠两边的时钟猜。
            // 【口径】这里的"首帧"= 渲染层真的 swap 过一帧（画面已上屏），
            // 不是"播放时钟起跳"—— 后者会早 1.5–2.8 s（真机实测）。
            com.kivo.cinema.net.NetDiag.log("MPV 首帧就绪")
            // 273 v15：首帧之后才起保温池（起播关键路径上的连接不能让给它）。
            startUrlWarmup()
            // 273 v15：预览内核同理 —— 它要自己开一条源站连接。
            prewarmThumbnails()
            if (session.durationUs > 0) durationMs = session.durationUs / 1000
            statusText = "正在播放"
            phase = PlayerPhase.Playing
            isPlaying = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).post(positionPoll)
            applyProbeSubtitle(session)
            applyProbeMpvOptions(session)
            // 354：同名外挂字幕交还内核原生 `sub-auto=exact`（支持全部文本格式与编码探测）。
            // 273 v35：落点 seek 取证钩子（URL 与 SMB 两条链路共用本函数 ⇒ 挂这里两边都覆盖）。
            // 【为什么必须有它】见 PlayerProbeLandingSeek.kt 的文件头：
            // `input swipe` 与 `input motionevent` 都进不去 App 的手势层 ⇒
            // Emby 链路的落点行为**无法脚本复现** ⇒ v26 才从未被验证过。
            applyProbeLandingSeek()
            // 起播后才有 video-params：此时更新 PiP 宽高比才是真值。
            updatePipParams()
            // `start=` 已经把内核放到续播位了，这里只补齐界面上的位置。
            if (resumeMs > 0L) {
                positionMs = resumeMs
            } else {
                applyResumePosition()
            }
            // 起播默认倍速（264 默认播放速度）：mpv 出厂 1.0，仅非默认值才下发。
            if (playbackPrefs.defaultSpeed != 1.0) applySpeed(playbackPrefs.defaultSpeed)
            showStartupSpec()
            // 280 · 首帧后 2 s 才落盘指标：`demuxer-cache-time`（前向可播时长）
            // 与丢帧计数要跑一会儿才有意义，首帧瞬间采是 0（那会被误读成"没有缓存"）。
            android.os.Handler(mainLooper).postDelayed({
                com.kivo.cinema.playback.PlaybackMetrics.finish(
                    applicationContext, session.nativeOrNull()
                )
            }, 2000)
        } else {
            statusText = "网络视频无法播放：${session.lastError}"
            phase = PlayerPhase.Failed("网络视频无法播放：${session.lastError.ifBlank { "未知错误" }}")
            isPlaying = false
            controlsVisible = true
            // 失败也要落盘（记录卡在哪一步），否则"为什么慢"永远查不到。
            com.kivo.cinema.playback.PlaybackMetrics.finish(
                applicationContext, null, "首帧失败：${session.lastError}"
            )
        }
    }, resumeMs / 1000.0, gate)
}

/** v3.1b：挂网络缩略图读取器（mpv 第二实例，SMB/HTTP 同构）。
 *  MediaExtractor 在无索引 MKV 上首请求即线性扫卡死（真机 handler busy 定论），
 *  网络源预览帧改走内核；本地索引源仍用 ThumbnailReader。
 *
 *  【源的归属】谁创建谁拥有：读者释放时会关掉自己注册的源。
 *  所以这里传进来的必须是**只给预览用**的那份源（SMB 是第二份 SmbByteSource，
 *  HTTP 是第二段 HttpOriginSource）—— 绝不能把主播放的源传进来。 */
internal fun PlayerComposeActivity.mountNetworkThumbnails(source: com.kivo.cinema.net.ByteSource) {
    // 主重试真重建：旧 reader 的内核/线程/代理先收（release 会关它注册的旧源），避免泄漏与抢链路。
    thumbnailReader?.release()
    netThumbSource = source
    val reader = com.kivo.cinema.thumbnail.MpvThumbnailReader(
        applicationContext, source, cacheDir,
        extraMpvOptions = probeThumbOptions(),
        // 274 · 实验开关：预览单次响应上限（--ei kivo.thumbMaxRangeKb N，0 = 旧行为）。
        maxRangeBytesOverride = intent.getIntExtra("kivo.thumbMaxRangeKb", 0).toLong() * 1024L,
    )
    thumbnailReader = reader
    // 预热期空框也要有字：内核起好前拖动，用户看到的是"准备中"而不是黑框猜谜。
    thumbnailUnavailableReason = "预览帧准备中…"
    reader.onThumbnailReady = { _, bitmap ->
        android.os.Handler(mainLooper).post {
            thumbnailBitmap = bitmap
            thumbnailUnavailableReason = ""
        }
    }
    reader.onUnavailable = { reason ->
        android.os.Handler(mainLooper).post { thumbnailUnavailableReason = reason }
    }
    // 【关掉拖动预览时不要预热】
    // 预热不是"顺手做一下"：它会起**第二个 mpv 内核**（多一个 4K HEVC 的
    // MediaCodec 解码器 + 一整套 libplacebo 上下文），并把片源从 0 顺序读到
    // 20–35 MB（真机 273-E：4K 片源上 ≈20 MiB / ≈17 s）。
    // 预览关着的时候，这些带宽和内存全是白花 —— 实测本机就关着预览，
    // 却在起播后十几秒里白下 26–38 MiB。
    if (playbackPrefs.scrubPreview) {
        // 273 v15：预热挪到**首帧之后**（见 [prewarmThumbnails]）——
        // 预览内核要自己开一条源站连接，而起播第一跳要的也是空连接，
        // 两条握手撞在一起时片头表头从 0.34 s 涨到 0.96–2.1 s。
        pendingThumbPrewarm = true
    }
}

/**
 * 预热预览内核 —— **首帧之后**才调（273 v15）。
 *
 * 【为什么关掉预览时不预热】预热不是"顺手做一下"：它会起**第二个 mpv 内核**
 * （多一个 4K HEVC 的 MediaCodec 解码器 + 一整套 libplacebo 上下文），
 * 并把片源从 0 顺序读到 20–35 MB（真机 273-E：4K 片源上约 20 MiB / 约 17 s）。
 * 预览关着的时候，这些带宽和内存全是白花 —— 实测本机就关着预览，
 * 却在起播后十几秒里白下 26–38 MiB。
 */
internal fun PlayerComposeActivity.prewarmThumbnails() {
    if (!pendingThumbPrewarm) return
    pendingThumbPrewarm = false
    val reader = thumbnailReader ?: return
    // 291：预览内核会开**第二条**源站连接并读片源 20–35 MiB（真机 273-E）。
    // 慢源站下它会把主链路饿死 ⇒ 按 275 §5.2「低优先级预取」经闸门延后。
    OriginSupplyPressure.defer { reader.prewarm() }
}
