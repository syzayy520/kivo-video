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
 * S3 播放通道验收（243 D3）：SMB → SmbByteSource → RangeFd → fd → 原生解码器。
 *
 * 为什么不用 UI 手势：播放页强制横屏，Compose Slider 在该机型上不暴露可点击的
 * 语义节点（`uiautomator dump` 里没有 SeekBar），adb 坐标点不中。这里直接驱动
 * 同一条生产链路，验"起播 + 位置推进 + seek 落点"三件事。
 *
 * 参数：
 * ```
 * -e smbHost 192.168.5.1 -e smbShare XiaoMi -e smbPath 视频/电影/xxx.mkv
 * ```
 */
class SmbPlaybackSeekInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0
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
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
        )
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: NetworkPlaybackSession? = null
        try {
            // localPath 用来把"本地文件"当对照组：同样的解码器与节拍，
            // 唯一差别是传输层（FileByteSource vs SMB）。seek 落点若只在 SMB 异常，
            // 问题就在传输层；两边一样则说明是解码器的既有行为。
            val localPath = args?.getString("localPath")?.takeIf { it.isNotBlank() }
            val source: com.kivo.cinema.net.ByteSource
            if (localPath != null) {
                source = com.kivo.cinema.net.FileByteSource(localPath)
                log("OPEN", "local $localPath size=${source.size()}")
            } else {
                val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
                val opened = SmbByteSource.open(address, credential, path)
                log("OPEN", "$path size=${opened.size()}")
                source = opened
            }

            texture = SurfaceTexture(0)
            val surf = Surface(texture)
            surface = surf
            val range = RangeFd(
                source,
                targetContext.getSystemService(StorageManager::class.java),
            )
            session = NetworkPlaybackSession(targetContext, range)

            case("起播（准备 + 首帧会话就绪）") {
                val ready = CountDownLatch(1)
                var result = -1
                session.start(surf) { code -> result = code; ready.countDown() }
                check(ready.await(60, TimeUnit.SECONDS)) { "起播超时（60 s）" }
                check(result == 0) { "起播失败：${session.lastError}" }
                log("START", "codec=${session.codecName} dur=${session.durationUs} audio=${session.hasAudio}")
            }

            case("位置推进（3 秒窗口）") {
                val first = session.positionUs
                Thread.sleep(3_000)
                val second = session.positionUs
                log("POSITION", "${first / 1000} ms → ${second / 1000} ms")
                check(second > first) { "位置没有推进：$first → $second" }
                check(second - first in 1_000_000..6_000_000) { "推进速度异常：${(second - first) / 1000} ms/3s" }
            }

            case("seek 到中段并落位") {
                // 252：可指定落点，用来区分"所有 seek 都不行"与"远距离 seek 才不行"。
                val target = args?.getString("seekTargetMs")?.toLongOrNull()?.times(1000L)
                    ?: (session.durationUs / 2)
                val requestId = session.seekTo(target)
                val deadline = System.currentTimeMillis() + 20_000
                var landing = -1L
                var error = ""
                while (System.currentTimeMillis() < deadline) {
                    val outcome = session.lastSeekResult
                    if (outcome != null && outcome.requestId == requestId) {
                        landing = outcome.landingUs
                        error = outcome.error
                        break
                    }
                    Thread.sleep(200)
                }
                // 落点记录与"实际播放位置"要分开看：原生侧 landing 取自
                // snapshot().seek.landing_us，MKV 大文件可能回 0 但实际已跳走。
                Thread.sleep(2_000)
                val actual = session.positionUs
                log("SEEK", "target=${target / 1000} ms landing=${landing / 1000} ms " +
                    "actual=${actual / 1000} ms error=$error")
                // P244 诊断：把原生 summary 带进报告。判"落点 25s 但实际 5s"要二分：
                //   - seek.first_frame_us / video_first_pts_us ≈ 25s  → 帧确实跳到 25s，
                //     位置不动是 pacer/时钟未 rebase 到 landing；
                //   - 这两项仍 ≈5s 或缺失                        → 帧没跳到 25s，
                //     问题在 demux 之后的送包/解码。
                // 只打印、不改任何播放逻辑；数据本就由 poll() 每 100ms 缓存，零额外 IO。
                log("SUMMARY", session.currentSummary().replace('\n', ' ').replace('\r', ' '))
                // 判据必须看**实际播放位置**，不能只看原生上报的 landing。
                // 244 实测：landing=25000 ms（关键帧对齐）、actual=5182 ms，旧判据
                // `landing > 0 || |actual-target| < 60s` 让这种"报告成功但没跳"直接过关。
                val toleranceUs = 5_000_000L
                check(kotlin.math.abs(actual - target) <= toleranceUs) {
                    "seek 未生效：target=${target / 1000} ms landing=${landing / 1000} ms " +
                        "actual=${actual / 1000} ms（允许 ±5 s）"
                }
            }

            case("seek 后继续推进") {
                val first = session.positionUs
                Thread.sleep(3_000)
                val second = session.positionUs
                log("RESUME", "${first / 1000} ms → ${second / 1000} ms")
                check(second > first) { "seek 后未恢复播放" }
            }

            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n"))
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                // 报告要带全三个字段：stage 由 SmbError 按 libsmb2 原文细分到 认证/共享，
                // reason 是用户看到的那句话，detail 才有 nt status 与库的原文。
                val detail = (e as? com.kivo.cinema.smb.SmbFailure)
                    ?.let { " stage=${it.stage.label} reason=${it.reason} detail=${it.detail}" }
                    .orEmpty()
                putString("stream", report.joinToString("\n") + "\nFAIL$detail ${e.stackTraceToString()}")
            })
        } finally {
            // P244 诊断：显式等待收尾完成，让 NetworkPlaybackSession.releaseNative 把
            // files/evidence/network-native-sync.json 写全（含 seek.video_first_pts_us）。
            // 只等收尾、不改播放逻辑；输出供"帧是否真跳到 landing"二分。
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    private fun log(tag: String, message: String) {
        val line = "P243 $tag $message"
        report += line
        println(line)
        android.util.Log.i("P243S3", "$tag $message")
    }
}
