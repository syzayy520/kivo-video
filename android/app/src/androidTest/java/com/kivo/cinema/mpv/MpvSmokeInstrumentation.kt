package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.SystemClock
import android.util.Log
import android.view.Surface
import java.io.File

/**
 * 253 P1 验收：libmpv 内核冒烟。
 *
 * 判据（对应 docs/design/backend-core/253-mpv-kernel-phases.md 的 P1）：
 *   1. 内核能创建并初始化；
 *   2. 能绑定 Surface（EGL 上下文 + mpv 渲染上下文都建起来）；
 *   3. 能打开一个媒体；
 *   4. **播放位置在推进**（画面真的在走，不是卡在 0）；
 *   5. 能 seek 到远处且位置跟着变。
 *
 * 只用**本地文件**：P1 的目的是证明"内核能跑"，把 SMB 风险留到 P2/P3。
 * 本地文件由外部推送到 `/sdcard/Android/data/com.kivo.cinema/files/media/`。
 */
class MpvSmokeInstrumentation : Instrumentation() {

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
        // 分段开关：`-e stage create|surface|play`。
        // 崩溃定位用 —— 一次只跑一段，缩小到具体哪一步炸的。
        val only = args?.getString("stage")
        fun want(name: String): Boolean = only.isNullOrBlank() || only == name
        try {
            if (want("create")) case("内核创建与初始化") {
                val session = MpvSession.create(targetContext)
                session.initialize("libmpv", ao = "", logFile = mpvLogPath())
                log.append("initialized handle=${session.nativeHandle}")
                session.close()
            }

            if (want("surface")) case("Surface 绑定") {
                // 单独验一次：EGL 上下文与 mpv 渲染上下文都在这一步建立，
                // 失败会直接抛（原因在异常消息里）。
                val texture = SurfaceTexture(0)
                val surface = Surface(texture)
                MpvSession.create(targetContext).use { session ->
                    session.initialize("libmpv", ao = "", logFile = mpvLogPath())
                    session.attachSurface(surface)
                    log.append(" surfaceAttached")
                }
                surface.release()
                texture.release()
            }

            if (want("play")) case("本地文件播放 + 位置推进 + seek") {
                val media = locateMedia()
                checkNotNull(media) { "没有找到本地测试媒体（推到 files/media/ 下）" }
                log.append(" media=${media.name} size=${media.length()}")

                val texture = SurfaceTexture(0)
                val surface = Surface(texture)
                MpvSession.create(targetContext).use { session ->
                    // 【ao=audiotrack】mpv 的 Android 音频输出通过 JNI 调 Java 的
                    // AudioTrack —— 需要 JavaVM 已注册（我们在 nativeCreate 里做了）。
                    // 用 "null" 就没有声音，但能验证内核其它部分；这里开真实输出。
                    session.initialize("libmpv", ao = "audiotrack", logFile = mpvLogPath())
                    session.attachSurface(surface)
                    session.loadFile(media.absolutePath)
                    log.append(" loaded")

                    // 位置必须推进。给 10 秒（起播要解码，首帧可能要几秒）。
                    val deadline = SystemClock.elapsedRealtime() + 10_000
                    var position = -1.0
                    while (SystemClock.elapsedRealtime() < deadline) {
                        Thread.sleep(250)
                        position = session.positionSeconds()
                        if (position > 0.3) break
                    }
                    log.append(" position=$position")
                    check(position > 0.3) { "播放位置没有推进（position=$position）" }

                    // seek 到 60 秒，位置必须跟着变。
                    // 【为什么用"接近"而不是">55"】
                    // mpv 的 seek 落点取决于关键帧位置（GOP=48帧=2秒时误差很小，
                    // 但片源不同误差不同）。判据应是"落到了目标附近"，
                    // 而不是某个写死的下界 —— 否则短片源（播完就 EOF）会误判。
                    val seekTarget = 60.0
                    session.seek(seekTarget, exact = false)
                    Thread.sleep(2_500)
                    val afterSeek = session.positionSeconds()
                    log.append(" afterSeek=$afterSeek")
                    check(afterSeek > seekTarget - 5.0) {
                        "seek 之后位置没有跳到目标附近（before=$position after=$afterSeek target=$seekTarget）"
                    }
                }
                surface.release()
                texture.release()
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

    /** mpv 日志落盘路径。app 私有目录，run-as 能读。 */
    private fun mpvLogPath(): String =
        File(targetContext.filesDir, "mpv-kernel.log").absolutePath

    /** 找本地测试媒体：app 私有目录的 files/media 下第一个视频文件。 */
    private fun locateMedia(): File? {
        val context = targetContext
        val roots = listOf(
            File(context.filesDir, "media"),
            File("/sdcard/Android/data/com.kivo.cinema/files/media"),
        )
        val preferred = listOf("mpv-test-3min.mkv", "local-seek.mkv", "mkv-audio.mkv")
        for (root in roots) {
            if (!root.isDirectory) continue
            for (name in preferred) {
                val candidate = File(root, name)
                if (candidate.isFile && candidate.length() > 0) return candidate
            }
            root.listFiles()?.firstOrNull { it.isFile && it.length() > 1024 }?.let { return it }
        }
        return null
    }
}
