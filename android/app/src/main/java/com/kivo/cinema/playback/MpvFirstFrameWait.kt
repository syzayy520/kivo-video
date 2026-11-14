package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.NetDiag

/**
 * 273 v16：等**画面真的上屏**。
 *
 * ## 为什么单独立一个文件
 * 「起播到底算不算成功」是一条**独立于会话**的判定规则：
 * 它有自己的口径（渲染层 swap 过一帧）、自己的兜底（纯音频媒体）、
 * 自己的时间上限。写在会话里会让会话同时管"起播编排"和"起播判定"，
 * 两种变化原因挤在一个文件里 ⇒ 拆出来。
 *
 * ## 口径（重要，别改回"播放位置 > 0"）
 * 内核的播放位置 > 0 只说明**时钟在走**，画面还没画出来。
 * 真机实测两者差 **1.5–2.8 s** —— 拿时钟起跳当首帧，
 * 等于在黑屏时就对外说"正在播放"，用户看到的是一段黑。
 * 现在以渲染层的 `first_frame` 标志为准（原生在成功的 `eglSwapBuffers` 之后置位）。
 */
internal object MpvFirstFrameWait {

    /** 等首帧的上限（毫秒）。真机起播 1.5–4.8 s ⇒ 30 s 是"内核真的坏了"的量级。 */
    const val TIMEOUT_MS = 30_000L

    /** 轮询间隔（毫秒）。 */
    private const val POLL_MS = 50L

    /**
     * 起播判定结果。
     *
     * @param positionSeconds 起播时刻的播放位置（秒）。
     * @param confirmed true = 画面**确认**上屏；false = 走了兜底（纯音频或超时未确认）。
     */
    class Result(val positionSeconds: Double, val confirmed: Boolean)

    /**
     * 等第一帧上屏。
     *
     * @param native 内核会话。
     * @param isCancelled 判断是否已被取消（取消时抛异常，不静默返回）。
     * @return 起播判定结果。
     * @throws IllegalStateException 超时且时钟也没起跳（内核真的没起来）。
     */
    fun await(native: MpvSession, isCancelled: () -> Boolean): Result {
        val deadline = System.currentTimeMillis() + TIMEOUT_MS
        var position = -1.0
        while (System.currentTimeMillis() < deadline) {
            if (isCancelled()) throw IllegalStateException("播放已取消")
            Thread.sleep(POLL_MS)
            if (native.firstFrameRendered()) return Result(native.positionSeconds(), true)
            position = native.positionSeconds()
            // 纯音频媒体永远等不到首帧 —— 那就只等时钟起跳，别把用户晾 30 秒。
            if (position > 0.0 && native.property("vid")?.trim() == "no") {
                return Result(position, false)
            }
        }
        // 兜底：画面没确认但时钟在走 ⇒ 放行，但**说清楚**（不许静默降级）。
        position = native.positionSeconds()
        check(position > 0.0) { "内核起播超时（position=$position）" }
        NetDiag.log("[prep] ⚠ 未确认首帧上屏（仅时钟起跳 position=$position）")
        return Result(position, false)
    }
}
