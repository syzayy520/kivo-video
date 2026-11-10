package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.storage.StorageManager
import android.view.Surface
import com.kivo.cinema.net.RangeFd
import com.kivo.cinema.playback.NetworkPlaybackSession
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 硬解帧率基准：预读开 / 关交替（251）。
 *
 * 背景：243 关掉硬解路径的预读，理由是"预读让帧率从 24.00 掉到 20.27、末尾出现 567 ms 卡顿"。
 * 但那是在块窗口会**双倍读取**的版本上测的（24 MiB 逻辑读拉 50 MiB，见
 * `docs/evidence/p251/window-double-fill`）。双倍读取已修，那条结论必须重测，
 * 不能拿旧数字继续关着一个可能有益的开关。
 *
 * 为什么要**同一次运行内交替**而不是跑两遍：这台 NAS 的吞吐有分钟的波动
 * （裸读 8.2–8.9 MB/s 都出现过），分两次跑无法区分"预读的差别"和"机器当时的状态"。
 * 交替 + 每段 20 s，四段一组（关→开→关→开），等于自带对照组。
 *
 * 判据不是吞吐（吞吐已经证明预读略低），而是**抖动**：
 * `dropped` 与 `pace_lateness_p99`。预读的价值就是把这些挡在解码线程之外。
 *
 * 参数：`-e smbHost ... -e smbShare ... -e smbPath ... -e windowSec 20 -e windows 4`
 */
class SmbPaceBenchInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var args: Bundle? = null

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        val host = args?.getString("smbHost") ?: "192.168.5.1"
        val share = args?.getString("smbShare") ?: "XiaoMi"
        val path = args?.getString("smbPath")
            ?: "视频/电影/The.Furious.2026.2160p.iTunes.WEB-DL.DDP.5.1.Atmos.HDR10+.H.265-CHDWEB.mkv"
        val windowSec = (args?.getString("windowSec") ?: "20").toInt()
        val windows = (args?.getString("windows") ?: "4").toInt()
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
        )

        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: NetworkPlaybackSession? = null
        var source: SmbByteSource? = null
        try {
            val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
            val opened = SmbByteSource.open(address, credential, path)
            source = opened
            log("OPEN", "size=${opened.size()}")
            opened.setPrefetchEnabled(false)

            texture = SurfaceTexture(0)
            val surf = Surface(texture)
            surface = surf
            val range = RangeFd(opened, targetContext.getSystemService(StorageManager::class.java))
            val playing = NetworkPlaybackSession(targetContext, range)
            session = playing

            val ready = CountDownLatch(1)
            var code = -1
            playing.start(surf) { c -> code = c; ready.countDown() }
            check(ready.await(60, TimeUnit.SECONDS)) { "起播超时（60 s）" }
            check(code == 0) { "起播失败：${playing.lastError}" }
            log("START", "codec=${playing.codecName} dur=${playing.durationUs / 1000} ms")

            var previous = Counter.parse(playing.lastSummary)
            for (index in 0 until windows) {
                // 关→开→关→开：单数段开预读。切换只翻一个开关，不解码器不重建。
                val enabled = index % 2 == 1
                opened.setPrefetchEnabled(enabled)
                Thread.sleep(1_500)  // 让上一段的余量排空，别把边界算进来

                val posBefore = playing.positionUs
                val bytesBefore = opened.deliveredBytes()
                val began = System.currentTimeMillis()
                Thread.sleep(windowSec * 1000L)
                val elapsed = (System.currentTimeMillis() - began) / 1000.0
                val posAfter = playing.positionUs
                val bytes = opened.deliveredBytes() - bytesBefore
                val now = Counter.parse(playing.lastSummary)

                val frames = now.frames - previous.frames
                val dropped = now.dropped - previous.dropped
                val mediaSec = (posAfter - posBefore) / 1_000_000.0
                previous = now

                log(
                    "WINDOW",
                    "#${index + 1} prefetch=${if (enabled) "on " else "off"} " +
                        "wall=${"%.1f".format(elapsed)}s media=${"%.1f".format(mediaSec)}s " +
                        "speed=${"%.2f".format(mediaSec / elapsed)}x " +
                        "frames=$frames dropped=$dropped " +
                        "fps=${"%.2f".format(frames / elapsed)} " +
                        "net=${"%.2f".format(bytes / 1024.0 / 1024.0 / elapsed)} MB/s " +
                        "need=${"%.2f".format(bytes * 8.0 / 1_000_000.0 / mediaSec.coerceAtLeast(0.1))} Mbps " +
                        "p99=${"%.1f".format(now.paceP99)}ms",
                )
            }
            log("SUMMARY", "codec=${playing.codecName} pipeSummary=${playing.lastSummary.replace('\n', ' ')}")
            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n"))
            })
        } catch (e: Throwable) {
            log("FAIL", "${e.javaClass.simpleName}: ${e.message}")
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\n")
            })
        } finally {
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { source?.close() }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
    }

    /** 只从原生 summary 里取三个累计计数，取不到就给 0（不猜）。 */
    private data class Counter(val frames: Long, val dropped: Long, val paceP99: Double) {
        companion object {
            private val FRAMES = Regex("""frames=(\d+)""")
            private val DROPPED = Regex("""dropped=(\d+)""")
            private val PACE = Regex("""pace_lateness_p99\[[^\]]*]=([\d.]+)""")

            fun parse(summary: String): Counter = Counter(
                frames = FRAMES.find(summary)?.groupValues?.get(1)?.toLongOrNull() ?: 0L,
                dropped = DROPPED.find(summary)?.groupValues?.get(1)?.toLongOrNull() ?: 0L,
                paceP99 = PACE.find(summary)?.groupValues?.get(1)?.toDoubleOrNull() ?: 0.0,
            )
        }
    }

    private fun log(tag: String, message: String) {
        report += "$tag $message"
        println("P251 $tag $message")
        android.util.Log.i("P251PACE", "$tag $message")
    }
}
