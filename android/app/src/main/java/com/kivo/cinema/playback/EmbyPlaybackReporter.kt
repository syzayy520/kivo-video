package com.kivo.cinema.playback

import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/**
 * 317 · **Emby 播放会话上报**（对标其他播放器的"实时更新"）。
 *
 * ## 为什么必须做
 * 详情页看到的「看到第几分钟」、换设备/换客户端的续播点，都存在 Emby 的
 * **该 item 的 UserData.PlaybackPositionTicks** 里。写它的唯一途径是播放会话三连：
 *
 * ```
 * POST /Sessions/Playing            开始（让服务器面板显示"正在播放"）
 * POST /Sessions/Playing/Progress   进度（每 10 s）
 * POST /Sessions/Playing/Stopped    停止 —— **必须带最后位置**，服务器据此落续播点
 * ```
 *
 * 只调 `setPlayed`（标已看/未看）不写位置，所以之前的记录永远是 0 ——
 * 这就是"别的播放器都有、我们没有"的原因。
 *
 * ## 为什么单独一个文件
 * "什么时候上报、节流多久、失败怎么办"是**策略**；
 * 拼 URL / 发 JSON 是**出网**（在 [EmbyClient] 里）。两种变化原因，分开。
 *
 * ## 三条纪律
 * 1. **不阻塞**：全部走后台线程。上报失败绝不能影响播放（播放器不依赖它）。
 * 2. **幂等**：`Playing` 每次会话只发一次；`Progress` 按 10 s 节流。
 * 3. **失败留痕**：连续失败只记一次日志，不刷屏（不能让日志变成噪音）。
 */
internal object EmbyPlaybackReporter {

    /** 进度上报间隔。Emby 官方客户端约 10 s，太密会被服务器当异常。 */
    private const val PROGRESS_INTERVAL_MS = 10_000L

    /** 一份会话：服务器地址、鉴权、条目、会话 id。 */
    private data class Session(
        val client: EmbyClient,
        val auth: EmbyClient.Auth,
        val itemId: String,
        val playSessionId: String?,
        /** 所属剧集 id（D366 缓存失效用）；电影 = itemId 本身，null = 未知。 */
        val seriesId: String?,
    )

    /** 从 intent 还原会话；任一必需项缺失就返回 null（非 Emby 片源）。 */
    private fun sessionOf(a: PlayerComposeActivity): Session? {
        val i = a.intent
        val server = i.getStringExtra(EXTRA_SERVER)?.takeIf { it.isNotBlank() } ?: return null
        val token = i.getStringExtra("kivo.embyToken")?.takeIf { it.isNotBlank() } ?: return null
        val userId = i.getStringExtra("kivo.embyUserId")?.takeIf { it.isNotBlank() } ?: return null
        val itemId = i.getStringExtra(EXTRA_ITEM)?.takeIf { it.isNotBlank() } ?: return null
        return Session(
            EmbyClient(server),
            EmbyClient.Auth(token, userId),
            itemId,
            i.getStringExtra("kivo.playSessionId"),
            i.getStringExtra(EXTRA_SERIES)?.takeIf { it.isNotBlank() },
        )
    }

    private fun posTicks(a: PlayerComposeActivity): Long = (a.positionMs * 10_000L).coerceAtLeast(0L)

    /** 起播后调用一次（幂等）。 */
    fun onStarted(a: PlayerComposeActivity) {
        if (a.embyPlayingReported) return
        val s = sessionOf(a) ?: return
        a.embyPlayingReported = true
        val ticks = posTicks(a)
        Thread({
            val ok = s.client.reportPlaying(s.auth, s.itemId, ticks, s.playSessionId)
            if (!ok) NetDiag.log("[emby] Playing 上报失败（不影响播放）")
            else NetDiag.log("[emby] Playing 上报 ok item=${s.itemId.take(10)} pos=${ticks / 10_000}ms")
        }, "KivoEmbyReport").start()
    }

    /** 位置轮询每 tick 调用（内部节流 10 s）。 */
    fun tick(a: PlayerComposeActivity) {
        if (!a.embyPlayingReported) return
        val now = android.os.SystemClock.uptimeMillis()
        if (now - a.embyReportedAtMs < PROGRESS_INTERVAL_MS) return
        val s = sessionOf(a) ?: return
        a.embyReportedAtMs = now
        val ticks = posTicks(a)
        Thread({
            s.client.reportProgress(s.auth, s.itemId, ticks, s.playSessionId)
        }, "KivoEmbyReport").start()
    }

    /**
     * 退出/暂停/播放结束时调用。
     *
     * 【为什么必须带位置】服务器只在 Stopped 的 PositionTicks 里落续播点；
     * 不带位置会回退到上一次 Progress（最多差 10 s，但长片反复进出会越差越多）。
     */
    fun onStopped(a: PlayerComposeActivity) {
        if (!a.embyPlayingReported) return
        val s = sessionOf(a) ?: return
        a.embyPlayingReported = false
        val ticks = posTicks(a)
        Thread({
            val ok = s.client.reportStopped(s.auth, s.itemId, ticks, s.playSessionId)
            NetDiag.log("[emby] Stopped 上报 ${if (ok) "ok" else "失败"} pos=${ticks / 10_000}ms")
            // D366：剧集详情缓存的是**整季的每集进度**，按单集 id 失效命不中 ⇒ 用所属剧集 id 失效。
            s.seriesId?.let { if (it != s.itemId) com.kivo.cinema.emby.EmbyResponseCache.invalidateItem(it) }
        }, "KivoEmbyReport").start()
    }

    /** intent key：当前播放条目的 Emby item id。 */
    const val EXTRA_ITEM = "kivo.embyItemId"

    /** intent key：所属剧集 id（D366，供停止后失效剧集详情缓存）。 */
    const val EXTRA_SERIES = "kivo.embySeriesId"

    /** intent key：服务器地址（含 `/emby` 前缀，[EmbyClient] 会自动拆）。 */
    const val EXTRA_SERVER = "kivo.embyServer"
}
