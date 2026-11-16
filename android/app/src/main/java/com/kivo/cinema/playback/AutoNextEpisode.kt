package com.kivo.cinema.playback

import android.content.Intent
import android.os.Handler
import android.os.Looper
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.switchEpisodeImmediately

/**
 * 317 · **播放结束后自动播下一集**。
 *
 * ## 为什么单独立一个文件
 * "播放结束之后该做什么"是一条**独立的策略**：它有自己的开关、自己的触发时机、
 * 自己的失败语义。塞进 [com.kivo.cinema.ui.compose.PlayerSessionBridge] 的位置轮询里，
 * 会让"采样状态"和"剧集推进"两种变化原因挤在同一个文件里（后者还要写 Handler、Intent）。
 *
 * ## 下一集信息从哪来（不查 API）
 * 详情页 `EmbyResource.episodeIntent()` 本来就知道整个剧集列表，启动播放时把
 * **下一集的 URL 与标签直接带进 intent**（`kivo.nextUrl` / `kivo.nextLabel`）。
 * 所以这里不需要 token、不需要查 Emby、不需要知道 SeriesId —— 一次往返都省掉。
 * 没有 `kivo.nextUrl` 就是最后一集（或非剧集）⇒ 保持"播放结束"。
 *
 * ## 为什么第一版是"重启 Activity"
 * 内核 `loadfile` 能无缝换片，但换集要重建整条起播链（`urlPrep` / 预热 / 首帧 /
 * HDR / 字幕 / 指标 / 预览），风险面远大于收益。第一版先走"新 Activity + finish 旧 Activity"，
 * 代价是 1~2 秒黑屏；**无缝切换是下一步优化**，届时复用 `startUrlPlayback` 的 generation 取消机制。
 *
 * ## 失败语义（不许静默）
 * · 没有下一集 → 什么都不做（正常结束）；
 * · Activity 已销毁 / 用户已退出 → 不启动；
 * · 启动失败（ActivityNotFound 等）→ 日志留痕，且状态栏回落成"播放结束"。
 */
internal object AutoNextEpisode {

    /** 倒计时时长：给用户留出"这是最后一集了/我要不要看下一集"的反应时间。 */
    private const val DELAY_MS = 3_000L

    private val handler = Handler(Looper.getMainLooper())

    /**
     * 播放自然结束时调用（**幂等**：同一次播放只会启动一次）。
     *
     * @param a 播放器 Activity
     * @return true = 已经安排了下一集；false = 没有下一集或条件不满足
     */
    fun maybeSchedule(a: PlayerComposeActivity): Boolean {
        if (a.autoNextScheduled) return false
        val nextUrl = a.intent.getStringExtra("kivo.nextUrl")?.takeIf { it.isNotBlank() } ?: return false
        val nextLabel = a.intent.getStringExtra("kivo.nextLabel").orEmpty()
        a.autoNextScheduled = true

        NetDiag.log("[autonext] 播放结束，${DELAY_MS}ms 后播放下一集：$nextLabel")
        val token = a.intent.getStringExtra("kivo.embyToken")
        val sourceName = a.intent.getStringExtra("kivo.sourceName")
        val sourceId = a.intent.getStringExtra("kivo.sourceId")
        val sizeHint = a.intent.getLongExtra("kivo.playSize", -1L)

        handler.postDelayed({
            if (a.isFinishing || a.isDestroyed) {
                NetDiag.log("[autonext] Activity 已退出，取消连播")
                return@postDelayed
            }
            // ① 优先**无缝换集**（同 Activity 内换源，不黑屏）—— 这是对标 SenPlayer 的关键。
            runCatching { a.switchEpisodeImmediately(nextUrl, nextLabel) }.getOrDefault(false).let { ok ->
                if (ok) {
                    NetDiag.log("[autonext] 无缝切到下一集")
                    return@postDelayed
                }
            }
            // ② 兜底：没有 SurfaceHolder 时回落到"新 Activity + finish 旧的"（黑 1~2 秒，但功能不缺）。
            runCatching {
                val next = Intent(a, PlayerComposeActivity::class.java)
                    .putExtra("kivo.playurl", nextUrl)
                    .putExtra("kivo.label", nextLabel)
                    .apply {
                        token?.let { putExtra("kivo.embyToken", it) }
                        sourceName?.let { putExtra("kivo.sourceName", it) }
                        sourceId?.let { putExtra("kivo.sourceId", it) }
                        if (sizeHint > 0L) putExtra("kivo.playSize", sizeHint)
                    }
                a.startActivity(next)
                a.finish()
                NetDiag.log("[autonext] 已回落重启 Activity 切到下一集")
            }.onFailure {
                NetDiag.log("[autonext] 启动下一集失败：${it.javaClass.simpleName}: ${it.message}")
                a.autoNextScheduled = false
            }
        }, DELAY_MS)
        return true
    }

    /** 用户手动操作（暂停/拖动/退出）时撤销待执行的连播。 */
    fun cancel(a: PlayerComposeActivity) {
        if (!a.autoNextScheduled) return
        handler.removeCallbacksAndMessages(null)
        a.autoNextScheduled = false
        NetDiag.log("[autonext] 用户介入，取消连播")
    }
}
