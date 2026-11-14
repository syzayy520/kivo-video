package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.mpv.MpvSpeedController
import com.kivo.cinema.mpv.MpvTrackCatalog
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 253-P6-2：seek 落点测量 —— mpv 的 seek 是**异步**的，"请求位置"不等于"落点"。
 *
 * ## 为什么单独一个类
 * mpv 的 seek 落点会**吸附到关键帧**：请求 14.014 s，实际可能落在 14.0 s。
 * 要把这个差异报给用户（进度条到底跳到哪了），就得等内核真的跳过去再读一次时钟 ——
 * 这段"等待 → 读回 → 判定落定"的逻辑与"会话起停"无关，
 * 与"内核的 seek 语义"绑死 ⇒ 跟着内核语义走，不跟着会话走。
 *
 * ## 怎么判定落定
 * 读回的落点与目标差 [LANDING_SLACK_US] 以内就算落定
 * （关键帧间隔通常远小于 3 s；差得更多说明这一跳还没发生，继续等）。
 * 最多等 [LANDING_TIMEOUT_MS] —— 宁可给一个"当前可见位置"，也不能让 UI 一直转圈。
 */
class MpvSeekControl(private val sessionProvider: () -> MpvSession?) {

    /**
     * 一次 seek 的结果。
     *
     * @param requestId 请求序号（上层用它把回调对上请求）。
     * @param targetUs 请求落点（微秒）。
     * @param landingUs 实际落点（微秒）；-1 = 没跳成。
     * @param error 失败原因（空串 = 成功）。
     */
    data class Result(
        val requestId: Long,
        val targetUs: Long,
        val landingUs: Long,
        val error: String = "",
    )

    private val ids = java.util.concurrent.atomic.AtomicLong()

    /** 取下一个请求序号。 */
    fun nextRequestId(): Long = ids.incrementAndGet()

    /**
     * 发一次 seek 并等落点。
     *
     * @param targetUs 目标位置（微秒）。
     * @param exact true = 精确到帧；false = 允许吸附到关键帧。
     * @return 落点结果（失败时 [Result.error] 非空、[Result.landingUs] = -1）。
     */
    fun seek(targetUs: Long, exact: Boolean): Result {
        val requestId = nextRequestId()
        val native = sessionProvider()
            ?: return Result(requestId, targetUs, -1, "内核会话尚未启动或已释放")
        return try {
            native.seek(targetUs / 1_000_000.0, exact = exact)
            Result(requestId, targetUs, awaitLanding(native, targetUs))
        } catch (e: Exception) {
            Result(requestId, targetUs, -1, "调整进度失败（${e.javaClass.simpleName}）")
        }
    }

    /**
     * 等内核真的跳过去，把落点读回来。
     *
     * 【为什么不能直接用请求值当落点】mpv 会吸附到关键帧，
     * 拿请求值冒充落点 = 对用户撒谎（进度条显示的位置和画面不是一回事）。
     */
    private fun awaitLanding(native: MpvSession, targetUs: Long): Long {
        val deadline = System.currentTimeMillis() + LANDING_TIMEOUT_MS
        var landing = targetUs
        while (System.currentTimeMillis() < deadline) {
            Thread.sleep(POLL_MS)
            val seconds = native.positionSeconds()
            if (seconds >= 0.0) {
                landing = (seconds * 1_000_000).toLong()
                if (kotlin.math.abs(landing - targetUs) < LANDING_SLACK_US) break
            }
        }
        return landing
    }

    companion object {
        /** 等落点的上限（毫秒）。 */
        const val LANDING_TIMEOUT_MS = 1_500L

        /** 落定判定容差（微秒）。 */
        const val LANDING_SLACK_US = 3_000_000L

        private const val POLL_MS = 100L
    }
}
