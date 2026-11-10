package com.kivo.cinema.playback

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import com.kivo.cinema.net.LocalRangeProxy
import com.kivo.cinema.net.ByteSource
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import java.io.File
import java.io.RandomAccessFile
import org.json.JSONObject

/** Synthetic Range service, production Compose host, existing native audio master clock. */
class NetworkAudioConnectionInstrumentation : Instrumentation() {
    private var fixtureName = "network-av.mp4"
    private var expectedAudioCodec = ""
    private var realSource = -1
    private var realItem = 0
    private var completeWithoutSeek = false
    private var realPreflight = true
    private var rangeOnly = false
    private var steadyOnly = false
    /** 稳态窗口。默认 20 s；`-e steadyMs 900000` 可拉到 15 min 做长稳。 */
    private var steadyMs = 20_000L
    override fun onCreate(arguments: Bundle?) {
        realSource = arguments?.getString("realSource")?.toIntOrNull() ?: -1
        realItem = arguments?.getString("realItem")?.toIntOrNull() ?: 0
        fixtureName = arguments?.getString("fixture") ?: fixtureName
        require(fixtureName.matches(Regex("[a-z0-9-]+\\.mp4")))
        expectedAudioCodec = arguments?.getString("audioCodec") ?: ""
        completeWithoutSeek = arguments?.getString("completeWithoutSeek") == "true"
        realPreflight = arguments?.getString("realPreflight") != "false"
        rangeOnly = arguments?.getString("rangeOnly") == "true"
        steadyOnly = arguments?.getString("steadyOnly") == "true"
        // 长稳窗口。上下界不是装饰：误传过大的值会把设备锁很久，
        // 过小则等于没测，两端都要挡住。
        steadyMs = (arguments?.getString("steadyMs")?.toLongOrNull() ?: 20_000L)
            .coerceIn(20_000L, 3_600_000L)
        require(!steadyOnly || realSource >= 0 && !completeWithoutSeek)
        super.onCreate(arguments); start()
    }
    override fun onStart() {
        val report = object : ArrayList<String>() {
            override fun add(element: String): Boolean {
                sendStatus(0, Bundle().apply { putString("stream", "$element\n") })
                return super.add(element)
            }
        }
        var host: PlayerComposeActivity? = null
        var network: NetworkPlaybackSession? = null
        var proxy: LocalRangeProxy? = null
        var stage = "source"
        try {
            report += RangeProbeRegression.run(targetContext)
            if (rangeOnly) {
                finish(Activity.RESULT_OK, Bundle().apply { putString("stream", report.joinToString("\n")) })
                return
            }
            val url = if (realSource >= 0) {
                val sources = com.kivo.cinema.sources.storage.SourceStore(targetContext)
                    .read().getOrThrow().filterNot { it.hidden }
                val source = sources[realSource]
                val client = com.kivo.cinema.emby.EmbyClient(source.address)
                stage = "login"
                val auth = client.login(source.username, source.password)
                stage = "media-list"
                require(realItem in 0..19)
                val item = client.items(auth, start = realItem, limit = 1).items.first()
                report += "REAL saved non-hidden source-${realSource + 1} item-${realItem + 1}; credentials and media identifiers omitted"
                client.streamUrl(auth, item.id)
            } else {
                val fixture = File(targetContext.getExternalFilesDir(null), "media/$fixtureName")
                check(fixture.isFile)
                val file = RandomAccessFile(fixture, "r")
                proxy = LocalRangeProxy(object : ByteSource {
                    override fun size() = file.length()
                    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
                        file.seek(offset)
                        return file.read(buf, off, len)
                    }
                    override fun close() { file.close() }
                })
                val url = "http://127.0.0.1:${proxy.start()}/synthetic"
                HttpRangeFile(targetContext, url).use { source ->
                    source.prepare()
                    check(source.size == fixture.length())
                    source.openDescriptor().use { first ->
                        source.openDescriptor().use { second ->
                            val a = ByteArray(4096); val b = ByteArray(4096)
                            val expected = ByteArray(4096)
                            RandomAccessFile(fixture, "r").use { local ->
                                local.seek(fixture.length() - 8192); local.readFully(expected)
                            }
                            check(Os.pread(first.fileDescriptor, a, 0, a.size, fixture.length() - 8192) == a.size)
                            check(Os.pread(second.fileDescriptor, b, 0, b.size, 0) == b.size)
                            check(a.contentEquals(expected))
                            check(!b.contentEquals(a))
                        }
                    }
                }
                report += "PASS independent proxy descriptors support exact random reads"
                url
            }

            stage = "range-comparison"
            if (realSource >= 0 && realPreflight) for (requested in listOf("0-0", "0-4095", "4096-8191", "0-")) {
                val c = java.net.URL(url).openConnection() as java.net.HttpURLConnection
                c.connectTimeout = 8000; c.readTimeout = 8000
                c.setRequestProperty("Range", "bytes=$requested")
                c.setRequestProperty("Accept-Encoding", "identity")
                try {
                    val code = c.responseCode
                    val range = c.getHeaderField("Content-Range") ?: "absent"
                    report += "range=$requested http=$code content_range=${range.takeIf { it.matches(Regex("bytes [0-9-]+/[0-9]+")) } ?: "invalid"} length=${c.contentLengthLong}"
                } finally { c.disconnect() }
            }
            if (realSource >= 0 && realPreflight) HttpRangeFile(targetContext, url).use { source ->
                stage = "range-prepare"
                source.prepare()
                report += "PASS real Range size=${source.size}"
                source.openDescriptor().use { fd ->
                    stage = "range-read"
                    val header = ByteArray(16)
                    val count = Os.pread(fd.fileDescriptor, header, 0, header.size, 0)
                    val kind = when {
                        header.copyOfRange(0, 4).contentEquals(byteArrayOf(0x1a, 0x45, 0xdf.toByte(), 0xa3.toByte())) -> "ebml"
                        String(header, 4, 4, Charsets.US_ASCII) == "ftyp" -> "mp4"
                        else -> "other"
                    }
                    report += "PASS real header read=$count container=$kind"
                    stage = "prefix-integrity"
                    val prefix = ByteArray(65536)
                    var copied = 0
                    while (copied < prefix.size) {
                        val n = Os.pread(fd.fileDescriptor, prefix, copied, prefix.size - copied, copied.toLong())
                        check(n > 0)
                        copied += n
                    }
                    val direct = java.net.URL(url).openConnection() as java.net.HttpURLConnection
                    direct.connectTimeout = 8000; direct.readTimeout = 8000
                    direct.setRequestProperty("Range", "bytes=0-${prefix.size - 1}")
                    direct.setRequestProperty("Accept-Encoding", "identity")
                    try {
                        check(direct.responseCode == 206)
                        val raw = ByteArray(prefix.size)
                        java.io.DataInputStream(direct.inputStream).use { it.readFully(raw) }
                        check(raw.contentEquals(prefix)) { "proxy bytes differ from direct range" }
                        report += "PASS proxy 64KiB prefix matches direct HTTP bytes"
                    } finally { direct.disconnect() }
                    val local = File.createTempFile("range-prefix-", ".bin", targetContext.cacheDir)
                    try {
                        local.writeBytes(prefix)
                        val prefixExtractor = android.media.MediaExtractor()
                        try {
                            prefixExtractor.setDataSource(local.absolutePath)
                            report += "CHECK truncated local 64KiB prefix tracks=${prefixExtractor.trackCount}"
                        } catch (e: java.io.IOException) {
                            report += "CHECK truncated local prefix unsupported (${e.javaClass.simpleName})"
                        } finally { prefixExtractor.release() }
                    } finally { local.delete() }
                    stage = "extractor-probe"
                    report += "CHECK real extractor probe (45-second I/O deadline)"
                    val extractor = android.media.MediaExtractor()
                    val deadline = java.util.Timer(true).apply {
                        schedule(object : java.util.TimerTask() {
                            override fun run() { source.cancel() }
                        }, 45_000)
                    }
                    try {
                        val beganProbe = SystemClock.elapsedRealtime()
                        extractor.setDataSource(fd.fileDescriptor, 0, source.size)
                        report += "CHECK extractor tracks=${extractor.trackCount} elapsed_ms=${SystemClock.elapsedRealtime() - beganProbe} io_reason=${source.failureReason.ifBlank { "none" }}"
                        check(extractor.trackCount > 0) { "no recognizable tracks" }
                        report += "PASS real extractor tracks=${extractor.trackCount}"
                        val types = (0 until extractor.trackCount).map {
                            extractor.getTrackFormat(it).getString(android.media.MediaFormat.KEY_MIME) ?: "unknown"
                        }
                        report += "CHECK track mimes=${types.joinToString(",")}"
                    } finally { deadline.cancel(); extractor.release() }
                }
            }

            val monitor = addMonitor(PlayerComposeActivity::class.java.name, null, false)
            val evidenceBeganAt = System.currentTimeMillis()
            stage = "host-launch"
            // Shell launch also works when OEM background-activity policy blocks Context.startActivity.
            // This API tokenizes directly, without shell metacharacter expansion. Reject whitespace
            // rather than adding quotes (which would become literal URL characters).
            check(url.none { it.isWhitespace() })
            uiAutomation.executeShellCommand("am start -W -n com.kivo.cinema/.ui.compose.PlayerComposeActivity --es kivo.playurl $url --es kivo.label network-test")
                .use { fd -> android.os.ParcelFileDescriptor.AutoCloseInputStream(fd).readBytes() }
            host = waitForMonitorWithTimeout(monitor, 15_000) as? PlayerComposeActivity
                ?: error("production host timeout")
            removeMonitor(monitor)
            stage = "prepare"
            val field = PlayerComposeActivity::class.java.getDeclaredField("smbNetSession").apply { isAccessible = true }
            eventually(90_000) {
                runOnMainSync { network = field.get(host) as? NetworkPlaybackSession }
                network?.state == NetworkPlaybackSession.State.PLAYING || network?.state == NetworkPlaybackSession.State.FAILED
            }
            val active = network!!
            check(active.state == NetworkPlaybackSession.State.PLAYING) { active.lastError }
            stage = "tracks"
            report += "CHECK production has_audio=${active.hasAudio} video_codec=${active.codecName}"
            check(active.hasAudio) { "audio track not recognized; audiovisual gate remains unmet" }
            check(!active.codecName.startsWith("OMX.google") && !active.codecName.startsWith("c2.android"))
            eventually { active.positionUs > 1_000_000 }
            report += "PASS production network host starts native hardware video and attaches audio"
            if (steadyOnly) {
                stage = "steady-without-operations"
                report += NetworkSteadyProgress.verify(active, steadyMs)
            } else {
            active.setPaused(true)
            stage = "pause"
            eventually { active.state == NetworkPlaybackSession.State.PAUSED }
            val paused = active.positionUs
            val rejectedSeek = active.seekTo(8_000_000)
            eventually { active.lastSeekResult?.requestId == rejectedSeek }
            check(active.lastSeekResult!!.error.isNotBlank())
            SystemClock.sleep(600)
            check(kotlin.math.abs(active.positionUs - paused) < 100_000)
            active.setPaused(false)
            stage = "resume"
            eventually { active.state == NetworkPlaybackSession.State.PLAYING && active.positionUs > paused + 200_000 }
            report += "PASS pause freezes media position and resume advances it"
            if (completeWithoutSeek) {
                stage = "steady-after-resume"
                report += NetworkSteadyProgress.verify(active, 6000)
                stage = "eos-after-pause"
                eventually(35_000) { active.state == NetworkPlaybackSession.State.ENDED || active.state == NetworkPlaybackSession.State.FAILED }
                check(active.state == NetworkPlaybackSession.State.ENDED) { active.lastError }
                report += "PASS natural EOS after pause/resume without seek"
            } else {
            stage = "seek-forward"
            report += seekAndVerify(active, 8_000_000)
            stage = "seek-backward"
            report += seekAndVerify(active, 2_000_000)
            report += "PASS forward and backward seek use the native A/V session"
            stage = "steady"
            report += NetworkSteadyProgress.verify(active, if (realSource >= 0) 20_000 else 3000)
            }
            }
            // 三个时刻各读一次 lastError：确定它是在 finish() 之前就有，
            // 还是关闭过程中被 cancel 路径写进去的。两次猜错之后，先量再改。
            val errorBeforeFinish = active.lastError
            runOnMainSync { host?.finish() }
            stage = "teardown"
            val errorAfterFinish = active.lastError
            val closeNotifications = java.util.concurrent.CountDownLatch(2)
            // 四个断言分开测、分开计时：合成一句 `check(...)` 时，失败只知道
            // 是 teardown，不知道是"关得慢"还是"通知没到"。
            val stopBegan = SystemClock.elapsedRealtime()
            repeat(2) { active.stop { closeNotifications.countDown() } }
            val stopReturned = SystemClock.elapsedRealtime()
            val closedOk = active.awaitClosed(15_000)
            val closeWaited = SystemClock.elapsedRealtime()
            val notifyOk = closeNotifications.await(1, java.util.concurrent.TimeUnit.SECONDS)
            val notifyWaited = SystemClock.elapsedRealtime()
            report += "CHECK teardown stop_ms=${stopReturned - stopBegan} " +
                "close_wait_ms=${closeWaited - stopReturned} closed=$closedOk " +
                "notify_wait_ms=${notifyWaited - closeWaited} notify=$notifyOk " +
                "err_pre=[${errorBeforeFinish}] err_fin=[${errorAfterFinish}] " +
                "err_final=[${active.lastError}]"
            check(closedOk) { "native teardown timeout" }
            check(notifyOk)
            check(active.lastError.isBlank()) { active.lastError }
            val evidence = File(targetContext.filesDir, "evidence/network-native-sync.json")
            check(evidence.lastModified() >= evidenceBeganAt) { "stale native evidence" }
            val json = JSONObject(evidence.readText())
            check(json.getString("schema") == "kivo.p12c.sync/45")
            val video = json.getJSONObject("video")
            val received = video.getLong("frames_received")
            val rendered = video.getLong("frames_rendered")
            val late = video.getLong("frames_dropped_late")
            val noSurface = video.getLong("frames_discarded_no_surface")
            val stopped = video.getLong("frames_discarded_on_stop")
            val stopUndelivered = video.getLong("frames_discarded_on_stop_undelivered")
            check(listOf(received, rendered, late, noSurface, stopped, stopUndelivered).all { it >= 0 })
            check(stopUndelivered <= stopped)
            check(received == video.getLong("frames_released")) { "decoder output slot leaked" }
            val eos = if (json.getString("end_reason") == "completed") 1L else 0L
            check(received == rendered + late + noSurface + stopped + eos) { "video slot accounting mismatch" }
            val pace = json.getJSONObject("pace")
            check(pace.getString("accounting_scope") == "session")
            val sessionStopped = pace.getLong("discarded_on_stop")
            val sessionNoSurface = pace.getLong("discarded_no_surface")
            check(sessionStopped >= stopUndelivered && sessionNoSurface >= noSurface)
            check(pace.getLong("frames_decided") - pace.getLong("presented") - pace.getLong("dropped") ==
                sessionNoSurface + sessionStopped) { "pace frame accounting mismatch" }
            report += "PASS video output slots and pace subset close exactly; repeated stop listeners delivered"
            val timeline = json.getJSONObject("timeline")
            check(timeline.getBoolean("media_rate_measured")) { "media rate not measurable" }
            check(timeline.getDouble("media_rate") in 0.98..1.02) { "media rate outside realtime gate" }
            report += "PASS report media_rate=${timeline.getDouble("media_rate")}"
            val bound = json.getJSONObject("drift_gate").getLong("limit_ns") +
                pace.getLong("frame_duration_ns")
            check(kotlin.math.abs(pace.getLong("clock_lead_ns")) <= bound) { "clock lead exceeds bound" }
            check(json.getString("clock_source") == "audio_timestamp")
            val audio = json.getJSONObject("audio")
            check(audio.getBoolean("opened") && audio.getBoolean("started"))
            val output = audio.getJSONObject("output")
            val audible = output.getLong("frames_rendered") - output.getLong("frames_silenced")
            check(audible > 0 && output.getLong("first_audible_frame") >= 0)
            val codec = json.getJSONObject("codec").getString("audio")
            if (expectedAudioCodec.isNotEmpty()) check(codec == expectedAudioCodec) { "unexpected audio implementation: $codec" }
            val feed = audio.getJSONObject("feed")
            check(feed.getLong("frames_decoded") > 0 && feed.getLong("frames_written") > 0 &&
                feed.getDouble("decoded_peak") > 0.0001)
            if (completeWithoutSeek) {
                check(json.getBoolean("completed") && eos == 1L)
                check(late == 0L && noSurface == 0L && stopped == 0L)
                check(feed.getBoolean("reached_eos"))
                check(feed.getLong("frames_dropped") == 0L)
                check(feed.getLong("frames_decoded") == feed.getLong("frames_written"))
                report += "PASS all decoded PCM written without pause loss"
            }
            report += "CHECK audio_codec=$codec decoded_frames=${feed.getLong("frames_decoded")} peak=${feed.getDouble("decoded_peak")} audible_frames=$audible"
            report += "CHECK prefill_budget_ms=${audio.getInt("prefill_budget_millis")} initial_wait_ms=${audio.getLong("prefill_initial_wait_millis")} max_wait_ms=${audio.getLong("prefill_max_wait_millis")}"
            check(json.getString("error").isEmpty())
            report += "PASS exit serializes native teardown; evidence confirms rendered audio and audio master clock"
        } catch (e: Throwable) {
            // teardown 段的断言消息是帧账/结构信息（如 "video slot accounting mismatch"），
            // 不含地址与账号，放行它才能定位到底哪一条不闭合。
            // 其余阶段继续保持隐去。
            val detail = if (stage == "teardown") " msg=${e.message?.take(120)}" else ""
            report += if (realSource >= 0) "FAIL stage=$stage type=${e.javaClass.simpleName} state=${network?.state}$detail (details withheld for real-source privacy)" else "FAIL ${e.javaClass.simpleName}: ${e.message}"
            if (e is NetworkSteadyProgress.Failure) report += "PROGRESS ${e.diagnostic}"
            if (realSource >= 0) network?.lastError?.takeIf {
                it.isNotBlank() && !it.contains("://") && !it.contains("api_key", true) && !it.contains("token", true)
            }?.let { report += "player_reason=$it" }
        } finally {
            network?.paceCurve()?.let { points ->
                report += "CHECK media_curve wall_ms:position_s=" + points.joinToString(",") { "${it.first}:${it.second}" }
            }
            runOnMainSync { host?.finish() }
            network?.let { it.stop(); it.awaitClosed(15_000) }
            network?.ioTrace()?.forEach { report += "IO $it" }
            runCatching { proxy?.close() }
        }
        finish(if (report.any { it.startsWith("FAIL") }) Activity.RESULT_CANCELED else Activity.RESULT_OK,
            Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }

    private fun seekAndVerify(active: NetworkPlaybackSession, targetUs: Long): String {
        val request = active.seekTo(targetUs)
        eventually(30_000) {
            active.lastSeekResult?.requestId == request || active.state == NetworkPlaybackSession.State.FAILED
        }
        val result = active.lastSeekResult
        check(result?.requestId == request && result.error.isEmpty()) { "seek request failed" }
        check(result!!.landingUs in 0..targetUs) { "invalid previous-sync landing" }
        eventually(10_000) {
            active.positionUs > result.landingUs + 250_000 || active.state == NetworkPlaybackSession.State.FAILED
        }
        check(active.state == NetworkPlaybackSession.State.PLAYING) { "seek did not resume playback" }
        return "PASS seek request=$request target_us=$targetUs landing_us=${result.landingUs} advances_after_completion"
    }

    private fun eventually(timeoutMs: Long = 8_000, predicate: () -> Boolean) {
        val deadline = SystemClock.uptimeMillis() + timeoutMs
        while (!predicate()) {
            check(SystemClock.uptimeMillis() < deadline) { "condition timeout" }
            SystemClock.sleep(50)
        }
    }
}
