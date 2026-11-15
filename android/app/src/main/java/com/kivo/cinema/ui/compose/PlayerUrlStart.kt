package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.net.HttpOriginSource
import com.kivo.cinema.net.PassthroughSource
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
            // 283 · HLS 直通：转码降码率路线。
            //
            // 【为什么这一路没有主源/预览源】转码流不走分段取数层（无总长、
            // 不支持 Range），所以没有 windowCache 可共享、也没有片尾预取；
            // 预览缩略图对转码流同样无意义（拖动预览需要 Range，转码分片不给）。
            // 直接把 [PassthroughSource] 交给内核，代理对它的每个请求回 302，
            // 清单与分片全部由 mpv 直连源站。
            val purl = prep?.passthroughUrl
            if (purl != null) {
                val engine = com.kivo.cinema.playback.PlaybackEngine.select()
                check(!cancelled.get())
                // 【PlaySessionId 必须先拿到】HLS 分片请求服务端强校验它：
                // 真机实测   不带/为空 → HTTP 400；自造 GUID → HTTP 404。
                // ⇒ 先调一次 PlaybackInfo 拿服务端发的会话 id，拼进 master URL ——
                //   Emby 会把它带进子清单，分片请求就自动合法。
                val finalUrl = fetchPlaySessionId(purl, token)?.let { psid ->
                    com.kivo.cinema.net.NetDiag.log("[prep] PlaySessionId=$psid")
                    purl + if (purl.contains('?')) "&PlaySessionId=$psid" else "?PlaySessionId=$psid"
                } ?: purl.also {
                    com.kivo.cinema.net.NetDiag.log("[prep] PlaySessionId 拿不到 ⇒ 分片会被拒（仍按原 URL 尝试）")
                }
                runOnUiThread {
                    if (cancelled.get() || finishRequested || isDestroyed ||
                        generation != networkGeneration || !surfaceReady
                    ) return@runOnUiThread
                    networkPreparationCancelled = null
                    if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                        com.kivo.cinema.playback.PlaybackMetrics.mark(
                            com.kivo.cinema.playback.PlaybackMetrics.T_SOURCE_READY,
                        )
                        startMpvPlayback(holder, PassthroughSource(finalUrl), prep)
                    } else {
                        // 旧内核（RangeFd 路径）要随机读；转码流给不了。
                        // 不许静默：明说原因。
                        statusText = "转码播放需要新版内核通路（当前内核不支持无总长流）"
                        phase = PlayerPhase.Failed(statusText)
                        isPlaying = false
                        controlsVisible = true
                    }
                }
                return@Thread
            }
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
                url, token, "thumb", THUMB_CAPACITY_BYTES, thumbChunkBytes(), 0,
                tailWindowBytes = 0,
                externalWindowCache = main.windowCache,
                // 274 · 实验开关：预览是否也在落点窗口在飞时让路（默认否）。
                yieldThumbToLanding = intent.getIntExtra("kivo.yieldThumbToLanding", 0) == 1,
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
                    // 280 · 指标：源站对象已建好（连接可能还在握手中，见下面的闸门说明）。
                    com.kivo.cinema.playback.PlaybackMetrics.mark(
                        com.kivo.cinema.playback.PlaybackMetrics.T_SOURCE_READY
                    )
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
 * 274 · 实验开关：预览源的分块大小（字节）。
 *
 * 【来源】Activity 的 intent `--ei kivo.thumbChunkKb N`。
 * 【默认】0 = 用 [THUMB_CHUNK_BYTES]（1 MiB，旧行为）。
 *
 * ## 为什么需要这个开关（要验证的假设）
 *
 * 装置实测（`tools/p274_lab.sh`，0.35 MB/s，45 秒，3 次落点跳）：
 * **`thumb` 标签下了 8.00 MiB**（8 次 × 1 MiB）—— 而它**只要 1 帧**。
 *
 * 上一轮已证伪"缩小预读环"（环不是决定者，因为 **carry 恒等于 chunkBytes**，
 * 缩环时 carry 会吃掉差额 ⇒ 每 reader 的预读量不变）。
 *
 * ⇒ **carry 才是杠杆**：`OriginPrefetchPump` 的 `carry = ByteArray(client.chunkBytes)`，
 * 它的职责是"把当前块整个读完，连接才能回 keep-alive 池"。
 * ⇒ **缩 chunk 会同步缩 carry** ⇒ 每条连接少下 (chunk − 实际需要) 字节。
 *
 * 【为什么要开关而不是直接改】这是个**取舍**（chunk 越小 ⇒ 源站请求次数越多 ⇒
 * 每次都要付一次表头）⇒ 必须 A/B 出数据再定，不能拍。
 */
private fun PlayerComposeActivity.thumbChunkBytes(): Long {
    val kb = intent.getIntExtra("kivo.thumbChunkKb", 0)
    return if (kb > 0) kb.toLong() * 1024L else THUMB_CHUNK_BYTES
}

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

/**
 * 283 · 向源站开一个播放会话（拿 PlaySessionId）。
 *
 * 【为什么必须】HLS 分片请求服务端强校验这个 id：真机实测
 *   · 不带 / 为空 → HTTP 400；自造 GUID → HTTP 404（服务端只认自己发过的）。
 * ⇒ 只能先调一次 PlaybackInfo。Emby 会把它带进子清单，分片请求自动合法。
 *
 * 【为什么放这而不是 EmbyClient】它需要的全部输入（服务器、itemId、token）
 * 都能从直通 URL 里推导 —— 这是 URL 起播路径的私有推导，不出 Activity。
 *
 * @return 有效的 PlaySessionId；null = 拿不到（调用方按原 URL 继续并留痕，不静默成功）。
 */
private fun fetchPlaySessionId(hlsUrl: String, token: String?): String? = runCatching {
    val itemId = Regex("/Videos/([^/]+)/").find(hlsUrl)?.groupValues?.get(1)
        ?: return null
    val server = hlsUrl.substringBefore("/Videos/")
    // 认证走 query 里的 api_key（直通 URL 自带）⇒ 不依赖调用方传了 token 头。
    // 真机实测：只带 token 头而 intent 没传 token 时会匿名 401，PlaySessionId 拿不到。
    val apiKey = Regex("[?&]api_key=([^&]+)").find(hlsUrl)?.groupValues?.get(1)
    val url = "$server/emby/Items/$itemId/PlaybackInfo?AutoOpenLiveStream=false" +
        (apiKey?.let { "&api_key=$it" } ?: "")
    val conn = java.net.URL(url).openConnection() as java.net.HttpURLConnection
    try {
        conn.connectTimeout = 10_000
        conn.readTimeout = 15_000
        token?.takeIf { it.isNotBlank() }?.let { conn.setRequestProperty("X-Emby-Token", it) }
        if (conn.responseCode !in 200..299) {
            com.kivo.cinema.net.NetDiag.log("[prep] PlaybackInfo HTTP ${conn.responseCode}")
            return null
        }
        val json = org.json.JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
        json.optString("PlaySessionId").takeIf { it.isNotBlank() }
    } finally {
        conn.disconnect()
    }
}.getOrNull()
