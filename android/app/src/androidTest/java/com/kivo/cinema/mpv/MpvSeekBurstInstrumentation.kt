package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.view.Surface
import com.kivo.cinema.net.FileByteSource
import com.kivo.cinema.playback.MpvPlaybackSession
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 300 · 连续 seek 压测（275 §6.2「连续 20 次 seek 无旧请求覆盖新目标」）。
 *
 * ## 口径必须写清楚
 * 当前 `seekTo` 是**同步等落点**（`MpvSeekControl.awaitLanding`，上限 1.5 s），
 * 调用线程会阻塞到内核真的跳过去。所以"旧请求覆盖新目标"在**并发**意义上不成立
 * ——不存在两个在途 seek。本测试要证明的是另外三件事：
 *
 * 1. **20 次乱序连发**（模拟用户快速拖动）每次都拿到有效落点，不卡死；
 * 2. **最终位置 = 最后一次目标**，不是中途某个旧目标残留；
 * 3. **暂停态 seek 不把会话误启动**（275 §4.2 第 4 条）。
 *
 * ⚠️ 本测试**不**证明"取消过期 seek"——当前实现没有取消机制，
 * 快速拖动会串行排队。这是已知 gap，见产出里的 `serial=true` 标注。
 *
 * 用法：
 * ```
 * adb shell am instrument -w -e mediaPath /sdcard/.../mpv-test-3min.mkv \
 *   -Pkivo.testRunner=com.kivo.cinema.mpv.MpvSeekBurstInstrumentation ...
 * ```
 */
class MpvSeekBurstInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0
    private var args: Bundle? = null

    /** 20 个乱序目标（秒）：刻意让相邻两次跨度大，模拟快速拖动。 */
    private val targetsSec = intArrayOf(
        10, 150, 30, 170, 60, 5, 120, 90, 165, 45,
        140, 20, 175, 75, 100, 15, 160, 55, 130, 110,
    )

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        super.onStart()
        val log = StringBuilder()
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: MpvPlaybackSession? = null
        try {
            val path = args?.getString("mediaPath")
                ?: "/sdcard/Android/data/com.kivo.cinema/files/media/mpv-test-3min.mkv"

            texture = SurfaceTexture(0)
            surface = Surface(texture)
            session = MpvPlaybackSession(targetContext, FileByteSource(path))

            val ready = CountDownLatch(1)
            var rc = -1
            session.start(surface, onReady = { r -> rc = r; ready.countDown() })
            check(ready.await(40, TimeUnit.SECONDS)) { "起播超时" }
            check(rc == 0) { "起播失败：${session.lastError}" }

            // 等首帧后位置推进，确保会话真的在播（否则 seek 的时序不具代表性）。
            val warm = System.currentTimeMillis() + 10_000
            while (System.currentTimeMillis() < warm && session.positionUs < 500_000) {
                Thread.sleep(100)
            }
            check(session.positionUs > 500_000) { "位置未推进（${session.positionUs}）" }

            case("20 次乱序连续 seek") {
                var worstErrMs = 0L
                var worstCostMs = 0L
                for ((i, sec) in targetsSec.withIndex()) {
                    val targetUs = sec * 1_000_000L
                    val t0 = System.currentTimeMillis()
                    session.seekTo(targetUs)
                    val cost = System.currentTimeMillis() - t0
                    val r = session.lastSeekResult
                    val errMs = if (r.landingUs > 0) kotlin.math.abs(r.landingUs - targetUs) / 1000 else -1
                    worstCostMs = maxOf(worstCostMs, cost)
                    if (errMs > worstErrMs) worstErrMs = errMs
                    log.append(
                        "  #${i + 1} target=${sec}s landing=${r.landingUs / 1000}ms " +
                            "err=${errMs}ms cost=${cost}ms errMsg=${r.error}\n"
                    )
                    check(r.landingUs > 0) { "第 ${i + 1} 次 seek 无有效落点（${r.error}）" }
                    // 落点容差 5 s：关键帧间隔通常 2–5 s，超过说明没跳到位。
                    check(errMs in 0..5_000) { "第 ${i + 1} 次落点偏差 ${errMs}ms 超 5s" }
                }
                log.append("  最坏落点偏差=${worstErrMs}ms 最坏耗时=${worstCostMs}ms serial=true\n")

                // ② 最终位置必须贴住最后一次目标（110 s），不能残留中途目标。
                Thread.sleep(1_500)
                val lastSec = targetsSec.last()
                val finalUs = session.positionUs
                val driftMs = kotlin.math.abs(finalUs - lastSec * 1_000_000L) / 1000
                log.append("  最终 position=${finalUs / 1000}ms（最后目标 ${lastSec}s）drift=${driftMs}ms\n")
                check(driftMs < 6_000) { "最终位置偏离最后目标 ${driftMs}ms" }
            }

            case("暂停态 seek 不误启动") {
                session.setPaused(true)
                Thread.sleep(600)
                val pausedBefore = session.state == MpvPlaybackSession.State.PAUSED
                session.seekTo(60_000_000L)
                Thread.sleep(800)
                val stillPaused = session.state == MpvPlaybackSession.State.PAUSED
                val posUs = session.positionUs
                log.append(
                    "  暂停生效=$pausedBefore seek 后仍暂停=$stillPaused position=${posUs / 1000}ms\n"
                )
                check(pausedBefore) { "setPaused(true) 未生效" }
                check(stillPaused) { "暂停态 seek 把会话误启动（state=${session.state}）" }
                check(kotlin.math.abs(posUs - 60_000_000L) < 6_000_000L) {
                    "暂停态 seek 落点不对（${posUs / 1000}ms）"
                }
            }
        } catch (e: Throwable) {
            finish(
                Activity.RESULT_CANCELED,
                Bundle().apply {
                    putString("stream", report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}")
                },
            )
            return
        } finally {
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
        finish(
            Activity.RESULT_OK,
            Bundle().apply {
                putString(
                    "stream",
                    report.joinToString("\n", postfix = "\n$passed/$total PASS\n") + "\n" + log,
                )
            },
        )
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }
}
