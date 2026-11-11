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
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 253-P6 验收：**内核控制面**（属性读写 / 轨道 / 字幕 / 倍速）。
 *
 * ## 为什么这轮测的是"控制面"
 * P1~P5 证明了"能播"。但换内核的已知代价是：
 *  · 字幕不显示（旧内核走 native 字幕句柄，mpv 侧没接）；
 *  · 倍速没有（旧内核有）；
 *  · 多轨切换没有（旧内核有）。
 *
 * 这三项**都只能通过 mpv 的属性通道实现**，而 P1~P5 时期
 * JNI 层**根本没有导出属性接口**（C++ 的 `Session::property` 实现了但 Kotlin 调不到）。
 * 所以本测试的第一件事是验证那条通道本身通了。
 *
 * ## 判据
 *  1. 属性通道：读 `track-list/count` 能拿到轨道数；
 *  2. 轨道目录：能列出音轨（Shelter 有两条 TrueHD）与字幕轨；
 *  3. 选轨：切换音轨后 `aid` 真的变了；
 *  4. 倍速：`speed` 能写能读，且**播放速度真的跟着变**
 *     （写进去不算数 —— 位置推进速率必须变，这是唯一能证伪的判据）；
 *  5. 外挂字幕：`sub-add` 能挂上，mpv 的字幕轨数随之 +1。
 *
 * ## 字幕"真的画出来了"怎么判
 * 本测试**不**判"字幕已显示" —— 那需要像素比对，而外挂字幕的挂载成功
 * 只证明管道通了，不证明画面上有字。
 * 画面上的字由 P6-2 的截图取证负责（同一批产物）。
 */
class MpvControlPlaneInstrumentation : Instrumentation() {

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
            case("内核控制面：属性 / 轨道 / 倍速 / 外挂字幕") {
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
                texture = SurfaceTexture(0)
                surface = Surface(texture)

                session = MpvPlaybackSession(targetContext, source)
                val ready = CountDownLatch(1)
                var result = -1
                session.start(surface) { r ->
                    result = r
                    ready.countDown()
                }
                check(ready.await(60, TimeUnit.SECONDS)) { "起播超时（60 s）" }
                check(result == 0) { "起播失败：${session.lastError}" }
                log.append(" START ok")

                // ---- 判据 1：属性通道 ----
                val catalog = session.tracks
                val count = catalog.tracks().size
                log.append(" TRACKS count=$count")
                check(count > 0) { "属性通道读不到轨道（track-list 为空）" }

                // ---- 判据 2：轨道目录能分类 ----
                val audio = catalog.audioTracks()
                val subs = catalog.subtitleTracks()
                log.append(
                    " AUDIO=${audio.size} SUB=${subs.size}" +
                        if (audio.isNotEmpty()) {
                            " aid1=${audio[0].id} codec=${audio[0].codec}"
                        } else "",
                )
                check(audio.isNotEmpty()) { "没识别出音轨" }

                // ---- 判据 3：切音轨 ----
                if (audio.size >= 2) {
                    val before = catalog.currentTrackId("audio")
                    val target = audio.first { it.id != before }
                    check(catalog.selectTrack("audio", target.id)) {
                        "选音轨失败（id=${target.id}）"
                    }
                    Thread.sleep(800)
                    val after = catalog.currentTrackId("audio")
                    log.append(" AID $before -> $after (target=${target.id})")
                    check(after == target.id) {
                        "选轨后 aid 没变（before=$before after=$after target=${target.id}）"
                    }
                } else {
                    log.append(" AID skipped（只有一条音轨）")
                }

                // ---- 判据 4：倍速（写进去 + 播放速度真的变）----
                // 【为什么必须量"位置推进速率"】
                // 只验证 `speed` 属性读回来是 1.5，证明不了内核真的在快放。
                // 唯一能证伪的判据是：同样的墙钟时间内，媒体时间走得更快。
                val rateBefore = measureRate(session)
                log.append(" RATE normal=${"%.2f".format(rateBefore)}x")
                check(session.setSpeed(2.0)) { "设倍速失败" }
                Thread.sleep(1_500)
                val speedReadback = session.currentSpeed()
                log.append(" SPEED readback=$speedReadback")
                check(kotlin.math.abs(speedReadback - 2.0) < 0.05) {
                    "speed 读回来不是 2.0（读到 $speedReadback）"
                }
                val rateFast = measureRate(session)
                log.append(" RATE fast=${"%.2f".format(rateFast)}x")
                check(rateFast > rateBefore * 1.4) {
                    "设了 2.0 倍但位置推进速率没变（${"%.2f".format(rateBefore)} -> " +
                        "${"%.2f".format(rateFast)}）"
                }
                session.setSpeed(1.0)

                // ---- 判据 5：外挂字幕 ----
                val subFile = writeTestSubtitle()
                val subsBefore = catalog.subtitleTracks().size
                check(session.loadExternalSubtitle(subFile)) {
                    "外挂字幕挂载失败：${session.lastError}"
                }
                Thread.sleep(1_500)
                val subsAfter = catalog.subtitleTracks().size
                val externalIds = catalog.externalSubtitleIds()
                log.append(
                    " SUB subsBefore=$subsBefore subsAfter=$subsAfter external=$externalIds",
                )
                check(externalIds.isNotEmpty()) {
                    "外挂字幕挂载后没有出现在字幕轨列表里（before=$subsBefore after=$subsAfter）"
                }
                // 挂上去之后必须能选中（mpv 不会自动选外挂字幕）。
                val extId = externalIds.first()
                check(session.selectSubtitle(extId)) { "选中外挂字幕失败（id=$extId）" }
                Thread.sleep(500)
                check(session.isSubtitleVisible()) { "外挂字幕挂上了但不可见" }
                log.append(" SUB selected=$extId visible=${session.isSubtitleVisible()}")
            }
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString(
                    "stream",
                    report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}",
                )
            })
            return
        } finally {
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
        finish(Activity.RESULT_OK, Bundle().apply {
            putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n") + "\n" + log)
        })
    }

    /**
     * 量"媒体时间 / 墙钟"的推进速率。
     *
     * @return 倍率（1.0 = 实时）。低于 1.0 说明在掉速。
     */
    private fun measureRate(session: MpvPlaybackSession): Double {
        val startWall = System.currentTimeMillis()
        val startPos = session.positionUs
        Thread.sleep(3_000)
        val wallUs = (System.currentTimeMillis() - startWall) * 1000L
        val mediaUs = session.positionUs - startPos
        return if (wallUs > 0) mediaUs.toDouble() / wallUs else 0.0
    }

    /**
     * 写一条测试外挂字幕。
     *
     * 【为什么要自己造一条而不是找现成的】
     * 片源里有没有字幕轨取决于原盘发行；而"外挂字幕管道通不通"
     * 与片源无关。用一条自己写的、**内容确定**的字幕，
     * 后面做截图取证时能确定画面上该出现哪句话。
     */
    private fun writeTestSubtitle(): File {
        val file = File(targetContext.filesDir, "kivo-test-sub.srt")
        file.writeText(
            buildString {
                append("1\n")
                append("00:00:00,000 --> 00:00:20,000\n")
                append("KIVO-MPV-SUBTITLE-PROBE\n")
                append("\n")
                append("2\n")
                append("00:00:20,000 --> 00:00:40,000\n")
                append("字幕轨道已接通\n")
                append("\n")
            },
        )
        return file
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    /**
     * 解析 `host` 或 `host:port` 形式的地址（平板经 PC 转发时端口不是 445）。
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
