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
 * 断网恢复验收（243 S3 补测 / 参照 F:\kivo 音乐播放器的重连纪律）。
 *
 * 验的是**一个用户能感知的行为**：播放中途 Wi-Fi 断 10 秒再回来，
 * 画面应该"卡住等一会儿然后接着播"，而不是"网络播放失败"。
 *
 * 为什么不在测试里切网络：切 Wi-Fi 需要 shell 权限，应用进程没有。
 * 所以测试只负责"起播 + 盯着位置变化"，断网由宿主在测试运行中
 * `svc wifi disable/enable` 完成；测试用"位置停住 ≥5 秒后又继续"来判定恢复。
 *
 * 判定分三步：
 * 1. 起播成功；
 * 2. 观察到一个 ≥5 秒的位置停顿（说明断网确实发生了）；
 * 3. 停顿之后位置继续推进，且会话状态始终是 PLAYING（不是 FAILED）。
 *
 * 参数：
 * ```
 * -e smbHost 192.168.5.1 -e smbShare XiaoMi -e smbPath 视频/电影/xxx.mkv
 * -e windowSec 120    观察窗口（默认 120 秒）
 * ```
 */
class SmbReconnectInstrumentation : Instrumentation() {

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
        val windowSec = args?.getString("windowSec")?.toIntOrNull() ?: 120
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
        )
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: NetworkPlaybackSession? = null
        try {
            val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
            val source = SmbByteSource.open(address, credential, path)
            log("OPEN", "$path size=${source.size()}")

            texture = SurfaceTexture(0)
            val surf = Surface(texture)
            surface = surf
            // 按生产接线传链路探针（250 D3）：否则测的是"没有看门狗"的旧行为。
            val range = RangeFd(
                source,
                targetContext.getSystemService(StorageManager::class.java),
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(targetContext) },
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

            // 观察窗口：宿主会在测试运行到约 20 秒时切网。
            var stallAt = -1L          // 停顿刚开始时的位置
            var stallFromMs = 0L       // 停顿开始的时间戳
            var resumeLatencyMs = -1L
            var failedReason = ""
            var sawBuffering = false   // 断网期间是否上报过"重连中"（界面据此提示）
            val began = System.currentTimeMillis()
            var last = session.positionUs
            var lastChangeAt = began
            // 252 S0：链路状态每 200 ms 采一次，把"恢复"这一刻钉死。
            // 只有钉住它，才能把"等在飞的读"(B) 和"重连"(C) 分开 —— 前者要改 JNI，
            // 后者改退避/连接参数就够，两者的修法完全不同。
            var wasOnline = true
            var offlineAt = 0L
            var onlineAt = 0L
            var resumeAfterOnlineMs = -1L
            while (System.currentTimeMillis() - began < windowSec * 1000L) {
                Thread.sleep(200)
                val online = runCatching {
                    com.kivo.cinema.net.NetworkLinkMonitor.isOnline(targetContext)
                }.getOrDefault(true)
                if (wasOnline && !online) {
                    wasOnline = false
                    offlineAt = System.currentTimeMillis()
                    log("LINK", "离线 @ ${offlineAt - began} ms（位置 ${last / 1000} ms）")
                } else if (!wasOnline && online) {
                    wasOnline = true
                    onlineAt = System.currentTimeMillis()
                    log("LINK", "恢复 @ ${onlineAt - began} ms（断网 ${onlineAt - offlineAt} ms）")
                }
                if (session.state == NetworkPlaybackSession.State.FAILED) {
                    failedReason = session.lastError
                    break
                }
                if (session.buffering && !sawBuffering) {
                    sawBuffering = true
                    // 记下"从位置停止到界面能提示"的间隔（250 G5 判据：≤5 s）。
                    // 位置停止时刻 ≈ 断网时刻，所以这个数是 AC8 的直接度量。
                    log("BUFFERING", "位置停止后 ${System.currentTimeMillis() - lastChangeAt} ms 首次上报重连中")
                }
                val now = System.currentTimeMillis()
                val pos = session.positionUs
                if (pos > last + 200_000L) {
                    if (stallAt >= 0 && resumeLatencyMs < 0 && pos > stallAt + 1_000_000L) {
                        resumeLatencyMs = now - stallFromMs
                        if (onlineAt > 0) resumeAfterOnlineMs = now - onlineAt
                        log("RESUME", "停 ${resumeLatencyMs} ms 后从 ${stallAt / 1000} ms 接上，现在 ${pos / 1000} ms" +
                            (if (onlineAt > 0) "；网络恢复后 ${resumeAfterOnlineMs} ms 接上" else ""))
                    }
                    last = pos
                    lastChangeAt = now
                } else if (stallAt < 0 && now - lastChangeAt >= 5_000L) {
                    stallAt = last
                    stallFromMs = lastChangeAt
                    log("STALL", "位置停在 ${stallAt / 1000} ms（已 ${now - lastChangeAt} ms 未推进）")
                }
            }
            log("TAIL", "state=${session.state} pos=${session.positionUs / 1000} ms " +
                "buffering=$sawBuffering err=${failedReason.ifEmpty { session.lastError }}")
            log("SPLIT", "断网 ${if (offlineAt > 0 && onlineAt > 0) onlineAt - offlineAt else -1} ms；" +
                "恢复后接上 ${resumeAfterOnlineMs} ms（判据 ≤2000）；" +
                "合计停顿 ${resumeLatencyMs} ms")

            case("断网时上报重连中状态") {
                check(sawBuffering) { "断网期间 buffering 始终为 false，界面无法提示\"正在重连\"" }
                check(session.buffering.not()) { "重连结束后 buffering 没有复位" }
            }

            case("断网期间会话没有判死") {
                check(failedReason.isBlank()) { "会话在断网期间失败：$failedReason" }
                check(session.state != NetworkPlaybackSession.State.FAILED) {
                    "会话最终状态 ${session.state}，错误：${session.lastError}"
                }
            }

            case("观察到断网造成的停顿") {
                check(stallAt >= 0) { "整段没有出现位置停顿，说明宿主没有成功切断网络" }
            }

            case("网络恢复后继续播放") {
                check(resumeLatencyMs >= 0) {
                    "停顿后未恢复推进：停在 ${stallAt / 1000} ms，当前 ${session.positionUs / 1000} ms"
                }
                log("RECOVERED", "停顿 ${resumeLatencyMs} ms 后恢复")
            }

            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n"))
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\nFAIL ${e.stackTraceToString()}")
            })
        } finally {
            runCatching { session?.stop() }
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
        android.util.Log.i("P243Reconnect", "$tag $message")
    }
}
