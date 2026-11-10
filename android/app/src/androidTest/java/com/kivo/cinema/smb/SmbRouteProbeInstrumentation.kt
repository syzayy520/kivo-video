package com.kivo.cinema.smb

import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.storage.StorageManager
import android.view.Surface
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.net.RangeFd
import com.kivo.cinema.playback.NetworkPlaybackSession
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 起播流程验收（252）：探测 → 决策 → 真正起播，三段都跑，逐段落盘。
 *
 * 为什么不用播放页做这件事：真机上 UI 侧不可靠（前台会被系统/用户切走、进程会被清），
 * 而这条流程的**逻辑**与 UI 无关 —— 同样的 `SmbByteSource → RangeFd →
 * FfmpegRouting.probe → NetworkPlaybackSession`。这里把它复刻一遍，
 * 判据是"探测必须在 5 s 内返回且结论为平台解码"+"起播后 20 s 内 0 丢帧"。
 *
 * 参数：`-e smbHost ... -e smbShare ... -e smbPath ...`
 */
class SmbRouteProbeInstrumentation : Instrumentation() {

    private lateinit var trace: File

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        val host = arguments?.getString("smbHost") ?: "192.168.5.1"
        val share = arguments?.getString("smbShare") ?: "XiaoMi"
        val path = arguments?.getString("smbPath")
            ?: "视频/电影/Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD/" +
            "Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD.mkv"

        trace = File(targetContext.filesDir, "p252-route.log").apply { writeText("") }
        var source: SmbByteSource? = null
        var range: RangeFd? = null
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: NetworkPlaybackSession? = null
        var passed = 0
        var total = 0
        try {
            val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)

            var t = System.nanoTime()
            val opened = SmbByteSource.open(address, SmbCredential("", ""), path)
            source = opened
            log("A-open", "size=${opened.size()} in ${ms(t)}")
            total++

            // 与 App 接线一致：探测用临时 fd（不拥有数据源），超时中断靠 cancel 它。
            val probeRange = RangeFd(
                opened, targetContext.getSystemService(StorageManager::class.java),
                ownsSource = false,
            )
            t = System.nanoTime()
            val decision = try {
                FfmpegRouting.probe(probeRange)
            } finally {
                runCatching { probeRange.close() }
            }
            val probeMs = (System.nanoTime() - t) / 1_000_000
            log("B-probe", "${decision.decision} (${decision.reason}) in ${probeMs} ms")
            total++
            if (probeMs <= 6_000L) passed++ else log("FAIL", "探测超过 6 s：${probeMs} ms")
            if (!decision.useFfmpeg) passed++ else log("FAIL", "路由落到软解：${decision.reason}")

            // 真正起播：与 App 硬解路径同一套接线（RangeFd + NetworkPlaybackSession）。
            val fd = RangeFd(opened, targetContext.getSystemService(StorageManager::class.java))
            range = fd
            texture = SurfaceTexture(0)
            val surf = Surface(texture)
            surface = surf
            val playing = NetworkPlaybackSession(targetContext, fd)
            session = playing

            val ready = CountDownLatch(1)
            var code = -1
            t = System.nanoTime()
            playing.start(surf) { c -> code = c; ready.countDown() }
            check(ready.await(60, TimeUnit.SECONDS)) { "起播超时（60 s）" }
            log("C-start", "code=$code codec=${playing.codecName} dur=${playing.durationUs / 1000} " +
                "ms in ${ms(t)} err=${playing.lastError}")
            total++
            if (code == 0) passed++ else log("FAIL", "起播失败 code=$code ${playing.lastError}")

            // 20 秒稳定性：判据是 0 丢帧、速度 1.00x。
            val before = Counter.parse(playing.lastSummary)
            val began = System.currentTimeMillis()
            Thread.sleep(20_000)
            val seconds = (System.currentTimeMillis() - began) / 1000.0
            val now = Counter.parse(playing.lastSummary)
            val frames = now.frames - before.frames
            val dropped = now.dropped - before.dropped
            val fps = frames / seconds
            log("D-play", "frames=$frames dropped=$dropped fps=${"%.1f".format(fps)} " +
                "pipe=${playing.lastSummary.replace('\n', ' ')}")
            total += 2
            if (dropped == 0L) passed++ else log("FAIL", "20 s 内丢帧 $dropped")
            if (fps >= 20.0) passed++ else log("FAIL", "fps 偏低：${"%.1f".format(fps)}")
            log("SUMMARY", "$passed/$total PASS")
        } catch (e: Throwable) {
            log("FAIL", "${e.javaClass.simpleName}: ${e.message}")
        } finally {
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { range?.close() }
            runCatching { source?.close() }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
    }

    /** 只从原生 summary 里取两个累计计数。 */
    private data class Counter(val frames: Long, val dropped: Long) {
        companion object {
            private val FRAMES = Regex("""frames=(\d+)""")
            private val DROPPED = Regex("""dropped=(\d+)""")
            fun parse(summary: String) = Counter(
                FRAMES.find(summary)?.groupValues?.get(1)?.toLongOrNull() ?: 0L,
                DROPPED.find(summary)?.groupValues?.get(1)?.toLongOrNull() ?: 0L,
            )
        }
    }

    private fun ms(since: Long): String = "%.0f ms".format((System.nanoTime() - since) / 1_000_000.0)

    private fun log(tag: String, message: String) {
        val line = "${System.currentTimeMillis()} $tag $message\n"
        runCatching { trace.appendText(line) }
        println("P252 $tag $message")
    }
}
