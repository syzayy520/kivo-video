package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.SystemClock
import android.view.Surface
import com.kivo.cinema.net.FileByteSource
import com.kivo.cinema.playback.MpvPlaybackSession
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 301 · 异步 seek 的「合并过期请求」验收。
 *
 * ## 验什么
 * 1. **不阻塞调用线程**：20 次 `seekToAsync` 的**提交总耗时**必须远小于单次 seek 成本
 *    （同步 `seekTo` 真机实测每次 100–216 ms ⇒ 20 次串行要 2 s 以上）。
 * 2. **中间目标被跳过**：单线程 worker 里只有票号最新的那次会真的发到内核
 *    ⇒ `seekExecuted` 应只有 **1–2**（第一次已在跑 + 最后一次），而不是 20。
 * 3. **最终落在最后目标**：旧票的结果不许覆盖新目标。
 *
 * ## 为什么这条重要
 * 275 §4.2 要求「取消或合并过期 seek」。300 的同步压测明确标注了 `serial=true`
 * ——那轮只证明"不卡死"，没证明"不排队"。本测试补的就是那一格。
 *
 * 用法（需带 `-Pkivo.testRunner=…MpvSeekAsyncInstrumentation` 编译测试包）：
 * ```
 * adb shell am instrument -w -e mediaPath /sdcard/.../mpv-test-3min.mkv \
 *   com.kivo.cinema.test/com.kivo.cinema.mpv.MpvSeekAsyncInstrumentation
 * ```
 */
class MpvSeekAsyncInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0
    private var args: Bundle? = null

    /** 20 个乱序目标；**最后一个是 165 s**，用来判"最终落在最后目标"。 */
    private val targetsSec = intArrayOf(
        10, 150, 30, 170, 60, 5, 120, 90, 175, 45,
        140, 20, 100, 75, 160, 15, 55, 130, 110, 165,
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
            val warm = System.currentTimeMillis() + 10_000
            while (System.currentTimeMillis() < warm && session.positionUs < 500_000) Thread.sleep(100)
            check(session.positionUs > 500_000) { "位置未推进" }

            case("20 次异步 seek：不阻塞 + 合并 + 落在最后目标") {
                // ① 提交耗时（这是"主线程会不会被占住"的直接度量）
                val t0 = SystemClock.elapsedRealtime()
                for (sec in targetsSec) session.seekToAsync(sec * 1_000_000L)
                val submitMs = SystemClock.elapsedRealtime() - t0
                log.append("  提交 20 次耗时=${submitMs}ms\n")
                check(submitMs < 300) { "提交阻塞过久（${submitMs}ms），异步没生效" }

                // ② 等最后一条落定（单次 seek 真机 ≤216ms，留 3 s 余量）
                Thread.sleep(3_000)
                val executed = session.seekExecuted
                val lastTarget = targetsSec.last() * 1_000_000L
                // 【判据用落点，不用 positionUs】positionUs 是**实时位置**，
                // 落定后视频会继续播 ⇒ 等 3 s 它当然已推进 3 s（第一次就是拿它判，
                // 得到 9.5 s 偏差而误判）。落点由 seek 结果写入，不受后续播放影响。
                val landing = session.lastSeekLandingUs
                val pos = session.positionUs
                val driftMs = kotlin.math.abs(landing - lastTarget) / 1000
                log.append(
                    "  实际执行=$executed 次（提交 20 次）\n" +
                        "  落点=${landing / 1000}ms 最后目标=${lastTarget / 1000}ms drift=${driftMs}ms\n" +
                        "  （参考：实时位置=${pos / 1000}ms，落定后继续播放所致）\n" +
                        "  lastSeekResult.targetUs=${session.lastSeekResult.targetUs / 1000}ms\n",
                )
                check(executed in 1..3) { "没有合并（执行了 $executed 次，期望 1–3）" }
                check(driftMs < 6_000) { "落点偏离最后目标 ${driftMs}ms" }
                check(session.lastSeekResult.targetUs == lastTarget) {
                    "lastSeekResult 不是最后目标（${session.lastSeekResult.targetUs / 1000}ms）"
                }
            }

            case("事件顺序：旧票结果不覆盖新目标") {
                // 先发一个远目标，紧接着发一个近目标；最终必须停在**后者**。
                val execBefore = session.seekExecuted
                session.seekToAsync(170_000_000L)
                session.seekToAsync(20_000_000L)
                Thread.sleep(2_500)
                // 同理用落点判断（positionUs 已随播放推进）。
                val landing = session.lastSeekLandingUs
                val pos = session.positionUs
                val drift = kotlin.math.abs(landing - 20_000_000L) / 1000
                log.append(
                    "  连发 170s→20s：落点=${landing / 1000}ms drift=${drift}ms " +
                        "（实时=${pos / 1000}ms）新增执行=${session.seekExecuted - execBefore}\n",
                )
                check(drift < 6_000) { "被旧目标 170s 覆盖了（落点 ${landing / 1000}ms）" }
            }
            case("主线程调用结束立即返回（这才是 UI 不卡的直接证据）") {
                // 【为什么要专门测主线程】前面那条是在 instrumentation 线程测的，
                // 只能证明"方法本身不阻塞"。真正的用户路径是 Compose 手势回调
                // = 主线程。同步版在这儿会占住 100–216 ms（真机 300 实测），
                // 异步版应只花"投递"的时间。
                var mainCost = 0L
                runOnMainSync {
                    val t = SystemClock.elapsedRealtime()
                    for (sec in targetsSec) session?.seekToAsync(sec * 1_000_000L)
                    mainCost = SystemClock.elapsedRealtime() - t
                }
                log.append("  主线程提交 20 次耗时=${mainCost}ms\n")
                check(mainCost < 200) { "主线程被占住 ${mainCost}ms" }
            }

        } catch (e: Throwable) {
            finish(
                Activity.RESULT_CANCELED,
                Bundle().apply {
                    putString(
                        "stream",
                        report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}",
                    )
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
