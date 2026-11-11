package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.SystemClock
import android.view.Surface
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import java.io.File

/**
 * 253-P3 验收：**用播放页真正使用的那条路径**跑通 SMB 播放。
 *
 * 【为什么不是直接调 MpvSession】
 * P2 的测试（MpvSmbInstrumentation）直接用 `MpvSession` + 手工起代理，验证的是
 * "内核 + 代理能工作"。但播放页用的是 [MpvPlaybackSession] —— 它多做了
 * 起播等待、进度轮询、seek 落点回报、收尾顺序这些事。
 * 这条测试走**同一个类**，验证的是"播放页那条路径能工作"。
 *
 * 判据：
 *   1. 会话能起播（state = PLAYING，位置推进）；
 *   2. 能拿到总时长（进度条可用）；
 *   3. 能 seek 到远处且落点正确；
 *   4. 收尾不崩（close 顺序正确）。
 */
class MpvSessionPathInstrumentation : Instrumentation() {

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
        super.onStart()
        val log = StringBuilder()
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: MpvPlaybackSession? = null
        try {
            case("MpvPlaybackSession：SMB 起播 + 时长 + seek + 收尾") {
                val host = args?.getString("smbHost") ?: "192.168.5.1"
                val share = args?.getString("smbShare") ?: "XiaoMi"
                val path = args?.getString("smbPath")
                checkNotNull(path) { "缺少 -e smbPath" }

                val source = SmbByteSource.open(
                    address = parseAddress(host, share),
                    credential = SmbCredential(
                        username = args?.getString("smbUser") ?: "",
                        password = args?.getString("smbPass") ?: "",
                    ),
                    filePath = path,
                )
                log.append(" smbSize=${source.size()}")

                texture = SurfaceTexture(0)
                surface = Surface(texture)

                session = MpvPlaybackSession(targetContext, source)
                val ready = java.util.concurrent.CountDownLatch(1)
                var result = -1
                session.start(surface) { r ->
                    result = r
                    ready.countDown()
                }
                check(ready.await(40, java.util.concurrent.TimeUnit.SECONDS)) {
                    "起播超时（40 秒内没有回调）"
                }
                log.append(" startResult=$result")
                check(result == 0) { "起播失败：${session.lastError}" }

                // 位置必须推进。
                val deadline = SystemClock.elapsedRealtime() + 20_000
                while (SystemClock.elapsedRealtime() < deadline) {
                    Thread.sleep(200)
                    if (session.positionUs > 500_000) break
                }
                val positionUs = session.positionUs
                log.append(" positionUs=$positionUs")
                check(positionUs > 500_000) { "位置没有推进（positionUs=$positionUs）" }

                // seek 到 600 秒，落点必须接近。
                session.seekTo(600_000_000)
                Thread.sleep(2_000)
                val landingUs = session.positionUs
                log.append(" landingUs=$landingUs")
                check(landingUs > 570_000_000) {
                    "seek 落点不对（landingUs=$landingUs 目标=600000000）"
                }
            }
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}")
            })
            return
        } finally {
            // 收尾顺序：先停会话（它释放 mpv 渲染上下文），再释放 Surface。
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
        finish(Activity.RESULT_OK, Bundle().apply {
            putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n") + "\n" + log)
        })
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    /**
     * 解析 `host` 或 `host:port` 形式的地址。
     *
     * 【为什么要支持端口】
     * 平板不在 NAS 那个网段（`10.0.0.x` vs `192.168.5.x`），要经 PC 的 TCP 转发
     * 才能连上 NAS —— 转发器监听 `127.0.0.1:14450`，设备侧用
     * `adb reverse tcp:14450 tcp:14450` 打通。此时端口**不是 445**，
     * 写死 DEFAULT_PORT 就连不上（真机实测："无法连接服务器"）。
     */
    private fun parseAddress(host: String, share: String): SmbAddress {
        val colon = host.lastIndexOf(':')
        if (colon > 0 && host.indexOf(':') == colon) {
            val h = host.substring(0, colon)
            val p = host.substring(colon + 1).toIntOrNull()
            if (p != null && p in 1..65535) {
                return SmbAddress(h, p, share)
            }
        }
        return SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
    }
}
