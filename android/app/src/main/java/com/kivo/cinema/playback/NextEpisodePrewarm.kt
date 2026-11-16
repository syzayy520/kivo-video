package com.kivo.cinema.playback

import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.net.OriginPrewarm
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/**
 * 317 · **预加载下一集**（对标 SenPlayer 的 `Preload Next Episode`）。
 *
 * ## 它解决的问题
 * 连播即使无缝换集，起播仍要重新握手 + 读容器头。真机量过：
 * 新 URL 的**第一跳表头 1000–1600 ms**，而预热过的是 **270–360 ms**（差 4 倍以上）。
 * 所以在片子快播完时就把下一集的片头抓进内存，切过去几乎立刻出画。
 *
 * ## 为什么复用 [OriginPrewarm] 而不是自己开连接
 * 详情页→播放页已经在用同一套预热（`EmbyResource.prewarm()` → `OriginPrewarm.offer()`），
 * 玩家页 `prepareUrlSource()` 会用 `OriginPrewarm.take(url)` **接手**已预热的源。
 * 复用它意味着：预热的连接读完会回 keep-alive 池 ⇒ 切集后的第一跳复用同一条 TCP。
 * 自己另开一套只会多一条连接、且接不上 `take()`。
 *
 * ## 触发时机
 * **剩余时长 ≤ 阈值**（默认 5 分钟），而不是"播放到 90%"。
 * 理由：90% 对 2 小时片是剩下 12 分钟（太早，白占带宽），对 22 分钟番剧是剩 2 分钟（刚好）。
 * 按**剩余时间**算对不同时长都合理。
 *
 * ## 与 SenPlayer 的差异（我们更克制）
 * SenPlayer 固定预取 **100 MB**。我们预取的是 `OriginPrewarm` 定义的那组窗口
 * （片头 4 MiB + 片尾 + 恢复点）——足以让首帧表头降到 ~300 ms，
 * 而不是在用户可能根本不看下一集时白下 100 MB。
 *
 * ## 边界
 * · 没有 `kivo.nextUrl`（最后一集）⇒ 什么都不做；
 * · 已经预热过 ⇒ [OriginPrewarm.offer] 内部幂等，不会重复开连接；
 * · 用户暂停/退出 ⇒ 预热不取消（它已经跑在后台线程，且切集时才 `take`），
 *   最坏结果是一份没被接手的缓存 —— 由 `OriginPrewarm.close` 在页面销毁时回收。
 */
internal object NextEpisodePrewarm {

    /** 剩余时长低于它就开始预加载（毫秒）。5 分钟 ≈ 一集番剧的 1/4。 */
    private const val LEAD_MS = 5 * 60 * 1000L

    /**
     * 由位置轮询每 tick 调用（幂等）。
     *
     * @return true = 本次真的发起了预热
     */
    fun maybeOffer(a: PlayerComposeActivity): Boolean {
        val nextUrl = a.intent.getStringExtra("kivo.nextUrl")?.takeIf { it.isNotBlank() } ?: return false
        if (a.durationMs <= 0L) return false
        if (a.durationMs - a.positionMs > LEAD_MS) return false
        if (a.nextPrewarmed) return false
        a.nextPrewarmed = true
        val token = a.intent.getStringExtra("kivo.embyToken")
        NetDiag.log(
            "[autonext] 剩余 ${(a.durationMs - a.positionMs) / 1000}s ⇒ 预加载下一集片头",
        )
        // sizeHint 未知（详情页没带下一集总长）⇒ -1，由 OriginPrewarm 自己探测。
        // 它不会因此变慢：prefetchHead 先发，prefetchTail 在拿到 size 后补发。
        OriginPrewarm.offer(nextUrl, token, sizeHint = -1L, resumeMs = 0L)
        return true
    }
}
