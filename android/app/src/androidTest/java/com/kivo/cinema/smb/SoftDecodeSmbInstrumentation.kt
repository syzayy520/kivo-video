package com.kivo.cinema.smb

import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.storage.StorageManager
import android.view.Surface
import com.kivo.cinema.bridge.FfmpegPlaybackSession
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.net.RangeFd
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 软解通道验收：SMB → SmbByteSource → RangeFd → FFmpeg 软解 → ANativeWindow。
 *
 * 为什么单独一个类而不是给 `SmbPlaybackSeekInstrumentation` 加参数：那条链路走的是
 * `NetworkPlaybackSession`（MediaCodec 硬解），软解是**另一套原生会话**
 * （`FfmpegLocalPlaybackSession`），错误码、证据 schema、控制面都不同。
 * 塞进同一个类只会变成一堆 `if (soft)`。
 *
 * 参数：
 * ```
 * -e smbHost 192.168.5.1 -e smbShare 视频 -e smbPath 电影/xxx.mkv
 * ```
 *
 * 验四件事：起播、位置推进、暂停冻结、seek 落点。
 */
class SoftDecodeSmbInstrumentation : Instrumentation() {

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
        val share = args?.getString("smbShare") ?: "视频"
        val path = args?.getString("smbPath")
            ?: "电影/[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT/" +
            "[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT.mkv"
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
        )

        var source: com.kivo.cinema.net.ByteSource? = null
        var range: RangeFd? = null
        var imageReader: android.media.ImageReader? = null
        var surface: Surface? = null
        var session: FfmpegPlaybackSession? = null
        val pumping = AtomicBoolean(true)

        try {
            // localPath 是本文件的本地副本作对照组：同一条软解会话、同一套回调，
            // 唯一差别是底层字节来自磁盘还是 SMB。用来区分"软解本身有问题"
            // 与"SMB 传输层把时间戳读坏了"。
            val localPath = args?.getString("localPath")?.takeIf { it.isNotBlank() }
            val opened: com.kivo.cinema.net.ByteSource = if (localPath != null) {
                com.kivo.cinema.net.FileByteSource(localPath)
            } else {
                SmbByteSource.open(
                    SmbAddress(host, SmbAddress.DEFAULT_PORT, share), credential, path
                )
            }
            source = opened
            log("OPEN", "${localPath ?: path} size=${opened.size()}")

            // 先量传输层本身：片源要实时播就得每秒钟稳定拿到 ~4 MB（32 Mbps）。
            // 这一条不达标，后面所有"解码慢"的结论都是错的。
            case("传输层吞吐（顺序读 24 MiB）") {
                val chunk = 64 * 1024        // 与 fd 回调层同粒度
                val buf = ByteArray(chunk)
                val budget = 24 * 1024 * 1024
                var offset = 0L
                var got = 0L
                val t0 = System.nanoTime()
                while (got < budget && offset < opened.size()) {
                    val n = opened.readAt(offset, buf, 0, chunk)
                    if (n <= 0) break
                    offset += n
                    got += n
                }
                val sec = (System.nanoTime() - t0) / 1_000_000_000.0
                val mbps = got / 1024.0 / 1024.0 / sec
                log("THROUGHPUT", "read=${got / 1024 / 1024} MiB in ${"%.2f".format(sec)}s " +
                    "= ${"%.2f".format(mbps)} MB/s")
            }

            val fdSource = RangeFd(opened, targetContext.getSystemService(StorageManager::class.java))
            range = fdSource

            // 路由判定必须在这条真实片源上跑一遍：它决定生产路径会不会进软解。
            val routing = FfmpegRouting.probe(fdSource)
            log("ROUTE", "decision=${routing.decision} useFfmpeg=${routing.useFfmpeg} reason=${routing.reason}")
            case("路由判定为软解（VC-1 无平台解码器）") {
                check(routing.useFfmpeg) { "判到了硬解：${routing.reason}" }
            }

            // 消费端用 ImageReader，**不用** SurfaceTexture。
            // 实测（2026-11-10）：SurfaceTexture.updateTexImage() 在 1080p 上
            // 每次要 ~20 ms 的 GL 拷贝，泵线程把一整个核吃满（top 里 96% CPU、
            // 17.9 s CPU 时间），把解码线程挤到一边 —— 于是量出来 0.28x 的
            // "解码太慢"里有一大半是**测量工具自己**在抢 CPU。
            // ImageReader 只需出队再关闭，没有 GPU 拷贝。
            val surfW = args?.getString("surfW")?.toIntOrNull() ?: 1920
            val surfH = args?.getString("surfH")?.toIntOrNull() ?: 1080
            val reader = android.media.ImageReader.newInstance(
                surfW, surfH, android.graphics.PixelFormat.RGBA_8888, 3)
            imageReader = reader
            surface = reader.surface
            val pump = Thread({
                while (pumping.get()) {
                    val image = runCatching { reader.acquireLatestImage() }.getOrNull()
                    if (image != null) image.close() else Thread.sleep(8)
                }
            }, "KivoTestSurfaceDrain").apply { isDaemon = true; start() }
            val live = FfmpegPlaybackSession(
                path.substringAfterLast('/'),
                { fdSource.openDescriptor() },
                fdSource.size,
                routing.decision,
                routing.reason,
            )
            session = live

            case("软解打开（demux + 解码器）") {
                val rc = live.openVideo(reader.surface)
                check(rc == 0) { "openVideo rc=$rc err=${live.nativeError()}" }
            }
            case("起解线程") {
                val rc = live.start()
                check(rc == 0) { "start rc=$rc err=${live.nativeError()}" }
            }
            // demux/probe 在 worker 线程里做（SMB 上一次 open 就是一次网络往返），
            // 主线程只能等。等不到就是真失败，不是慢。
            case("demux 初始化（20 秒内探到时长）") {
                val deadline = System.currentTimeMillis() + 20_000
                while (System.currentTimeMillis() < deadline && live.durationUs() <= 0) {
                    Thread.sleep(200)
                }
                log("DUR", "durationUs=${live.durationUs()}")
                check(live.durationUs() > 0) { "durationUs=${live.durationUs()}，worker 未完成探测" }
            }
            val durationUs = live.durationUs()
            case("位置推进（3 秒内前进 > 1 秒）") {
                val before = live.positionUs()
                Thread.sleep(3000)
                val after = live.positionUs()
                log("POS", "before=$before after=$after")
                check(after > before) { "位置没动：$before → $after" }
                check(after - before > 1_000_000) { "推进太慢：${(after - before) / 1000} ms" }
            }
            case("暂停后位置冻结") {
                check(live.setPaused(true) == 0) { "setPaused(true) 失败" }
                Thread.sleep(300)
                val anchor = live.positionUs()
                Thread.sleep(1200)
                val later = live.positionUs()
                log("PAUSE", "anchor=$anchor later=$later")
                check(later == anchor) { "暂停后仍在推进：$anchor → $later" }
            }
            val midUs = durationUs / 2
            case("暂停状态下 seek 到中点") {
                live.seekTo(midUs)
                Thread.sleep(2500)
                val landing = live.positionUs()
                log("SEEK", "target=$midUs landing=$landing diff=${(landing - midUs) / 1000} ms")
                check(landing > 0) { "落点无效：$landing" }
                // PREVIOUS_SYNC 语义下允许向后偏一个 GOP；容差 30 秒覆盖长 GOP 片源。
                check(kotlin.math.abs(landing - midUs) < 30_000_000L) {
                    "落点偏离目标 ${(landing - midUs) / 1000} ms"
                }
            }
            case("恢复后继续推进") {
                check(live.setPaused(false) == 0) { "setPaused(false) 失败" }
                val before = live.positionUs()
                Thread.sleep(2500)
                val after = live.positionUs()
                log("RESUME", "before=$before after=$after")
                check(after > before) { "恢复后位置没动：$before → $after" }
            }
            // 只测"位置会动"是不够的：会动但每秒只走 0.3 秒，成品就是幻灯片。
            // 判据用**媒体秒 / 墙钟秒**，1.0 才是真正跟得上。这条只取证不判死。
            case("持续播放 10 秒的实际速率（媒体秒 ÷ 墙钟秒）") {
                val p0 = live.positionUs()
                val t0 = System.nanoTime()
                Thread.sleep(10_000)
                val p1 = live.positionUs()
                val t1 = System.nanoTime()
                val mediaS = (p1 - p0) / 1_000_000.0
                val wallS = (t1 - t0) / 1_000_000_000.0
                val ratio = if (wallS > 0) mediaS / wallS else 0.0
                log("RATE", "media=${"%.2f".format(mediaS)}s wall=${"%.2f".format(wallS)}s " +
                    "ratio=${"%.2f".format(ratio)}x")
            }

            // ---- 音频：另一条独立链路（自己的 fd / 自己的 demux / 自己的解码线程）----
            // 用户的验收语是"只有声音"或"只有画面"，所以这两条必须各测一遍。
            val audioFd = fdSource.openDescriptor()
            val audio = com.kivo.cinema.playback.SoftAudioSession(
                path.substringAfterLast('/'), audioFd, fdSource.size)
            try {
                case("音频链路起得来") {
                    val rc = audio.start()
                    check(rc == 0) { "start rc=$rc ${audio.describeFailure()}" }
                }
                case("音频 20 秒内真正出声（AAudio 流已开）") {
                    val deadline = System.currentTimeMillis() + 20_000
                    while (System.currentTimeMillis() < deadline && !audio.hasOutput()) {
                        Thread.sleep(250)
                    }
                    val stats = audio.statsJson()
                    log("AUDIO", "hasOutput=${audio.hasOutput()} stats=$stats")
                    check(audio.hasOutput()) { "AAudio 没开起来：$stats" }
                    // 编解码器必须是我们补进来的那几个之一：TrueHD / AC3 / DTS / FLAC。
                    check(Regex("\"codec\":\"(truehd|mlp|ac3|eac3|dca|flac|opus|vorbis|mp2|alac|aac_latm)\"")
                        .containsMatchIn(stats)) { "解到的不是预期音轨：$stats" }
                }
                case("音频帧持续写入（不是开流后卡住）") {
                    val first = audio.statsJson()
                    Thread.sleep(2000)
                    val second = audio.statsJson()
                    log("AUDIO-FRAMES", "t0=$first")
                    log("AUDIO-FRAMES", "t1=$second")
                    fun frames(json: String): Long =
                        Regex("\"frames_written\":(\\d+)").find(json)
                            ?.groupValues?.get(1)?.toLongOrNull() ?: 0L
                    check(frames(second) > frames(first)) {
                        "2 秒内没有新 PCM 写入：${frames(first)} → ${frames(second)}"
                    }
                }
                case("音频暂停后位置冻结") {
                    audio.setPaused(true)
                    Thread.sleep(400)
                    val anchor = audio.positionUs()
                    Thread.sleep(1200)
                    val later = audio.positionUs()
                    log("AUDIO-PAUSE", "anchor=$anchor later=$later")
                    check(later == anchor) { "暂停后音频仍在推进：$anchor → $later" }
                }
                case("音频恢复后继续") {
                    audio.setPaused(false)
                    val before = audio.positionUs()
                    Thread.sleep(2000)
                    val after = audio.positionUs()
                    log("AUDIO-RESUME", "before=$before after=$after")
                    check(after > before) { "恢复后音频没动：$before → $after" }
                }
            } finally {
                runCatching { audio.close() }
                runCatching { audioFd.close() }
            }
        } catch (e: Exception) {
            log("FATAL", "${e.javaClass.simpleName}: ${e.message}")
            total++
        } finally {
            pumping.set(false)
            // 证据要在 close 之前拿：close 会先 stop，状态就只剩销毁流程了。
            runCatching {
                val evidence = session?.stopAndCollectEvidence()
                if (!evidence.isNullOrBlank()) {
                    log("EVIDENCE", evidence.replace("\n", " ").take(1800))
                }
            }
            runCatching { session?.close() }
            runCatching { surface?.release() }
            runCatching { imageReader?.close() }
            runCatching { range?.close() }
            runCatching { source?.close() }
            log("RESULT", "$passed/$total")
            finish(if (passed == total && total > 0) 0 else -1, Bundle().apply {
                putString("result", "$passed/$total")
            })
        }
    }

    private fun case(name: String, body: () -> Unit) {
        total++
        try {
            body()
            passed++
            log("PASS", name)
        } catch (e: Exception) {
            log("FAIL", "$name → ${e.message}")
        }
    }

    private fun log(tag: String, message: String) {
        android.util.Log.i("KivoSoftDecode", "[$tag] $message")
    }
}
