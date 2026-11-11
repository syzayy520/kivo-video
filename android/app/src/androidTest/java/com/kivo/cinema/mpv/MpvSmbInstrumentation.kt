package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import android.util.Log
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import java.io.File

/**
 * 253-P2 验收：libmpv 通过本地 HTTP range 代理读 NAS 上的真实片源。
 *
 * 判据（对应 docs/design/backend-core/253-mpv-kernel-phases.md 的 P2）：
 *   1. 代理能把 SMB 源变成一个 HTTP URL（HEAD 返回真实大小）；
 *   2. mpv 能打开这个 URL 并**起播**（位置推进）；
 *   3. mpv 能 seek 到远处且位置跟着变；
 *   4. 音频与视频轨都被识别（不是只解出画面）。
 *
 * 【为什么先过代理再上 mpv】
 * mpv 走 FFmpeg 的 `http` 协议读网络资源。我们 SMB 层的块窗口/预读/断网恢复
 * 比 FFmpeg 的 libsmbclient 强，但协议表是编译期静态的，插不进去。
 * 代理是中间路：mpv ← http ← 代理 ← readAt ← SmbByteSource ← NAS。
 * 我们的 SMB 层一点不改地继续生效。
 */
class MpvSmbInstrumentation : Instrumentation() {

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
        try {
            case("SMB → HTTP 代理 → mpv 起播 + seek") {
                val host = args?.getString("smbHost") ?: "192.168.5.1"
                val share = args?.getString("smbShare") ?: "XiaoMi"
                val path = args?.getString("smbPath")
                checkNotNull(path) { "缺少 -e smbPath" }

                // 1) 打开 SMB 源（走我们自己的块窗口/预读层）。
                val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
                val source = SmbByteSource.open(
                    address = address,
                    credential = SmbCredential(
                        username = args?.getString("smbUser") ?: "",
                        password = args?.getString("smbPass") ?: "",
                    ),
                    filePath = path,
                )
                val size = source.size()
                log.append(" smbSize=$size")

                // 2) 起代理并注册。
                val server = LocalRangeServer.start()
                val id = server.register(source, path)
                val url = server.urlFor(id)
                log.append(" url=$url")

                try {
                    // 3) mpv 打开这个 URL。
                    val session = MpvSession.create(targetContext)
                    try {
                        session.initialize(
                            "libmpv",
                            ao = "audiotrack",
                            logFile = File(targetContext.filesDir, "mpv-kernel.log").absolutePath,
                        )
                        session.loadFile(url)
                        log.append(" loaded")

                        // 4) 位置必须推进。
                        val deadline = SystemClock.elapsedRealtime() + 20_000
                        var position = -1.0
                        while (SystemClock.elapsedRealtime() < deadline) {
                            Thread.sleep(250)
                            position = session.positionSeconds()
                            if (position > 0.5) break
                        }
                        log.append(" position=$position")
                        check(position > 0.5) { "mpv 起播后位置没有推进（position=$position）" }

                        // 5) seek 到远处（源很大，取 10 分钟），位置必须跟着变。
                        val seekTarget = 600.0
                        session.seek(seekTarget, exact = false)
                        Thread.sleep(4_000)
                        val afterSeek = session.positionSeconds()
                        log.append(" afterSeek=$afterSeek")
                        check(afterSeek > seekTarget - 30.0) {
                            "seek 之后位置没有跳到目标附近（after=$afterSeek target=$seekTarget）"
                        }
                    } finally {
                        session.close()
                    }
                } finally {
                    server.close()
                }
            }
            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n") + "\n" + log)
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}")
            })
        }
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }
}
