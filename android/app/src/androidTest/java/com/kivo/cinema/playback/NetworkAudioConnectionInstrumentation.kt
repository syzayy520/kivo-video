package com.kivo.cinema.playback

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import com.kivo.cinema.net.LocalRangeProxy
import com.kivo.cinema.smb.SmbSource
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import java.io.File
import java.io.RandomAccessFile
import org.json.JSONObject

/** Synthetic Range service, production Compose host, existing native audio master clock. */
class NetworkAudioConnectionInstrumentation : Instrumentation() {
    private var realSource = -1
    private var realItem = 0
    override fun onCreate(arguments: Bundle?) {
        realSource = arguments?.getString("realSource")?.toIntOrNull() ?: -1
        realItem = arguments?.getString("realItem")?.toIntOrNull() ?: 0
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
                val fixture = File(targetContext.getExternalFilesDir(null), "media/network-av.mp4")
                check(fixture.isFile)
                val file = RandomAccessFile(fixture, "r")
                proxy = LocalRangeProxy(object : SmbSource.ByteSource {
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
            if (realSource >= 0) for (requested in listOf("0-0", "0-4095", "4096-8191", "0-")) {
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
            if (realSource >= 0) HttpRangeFile(targetContext, url).use { source ->
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
            active.setPaused(true)
            stage = "pause"
            eventually { active.state == NetworkPlaybackSession.State.PAUSED }
            val paused = active.positionUs
            SystemClock.sleep(600)
            check(kotlin.math.abs(active.positionUs - paused) < 100_000)
            active.setPaused(false)
            stage = "resume"
            eventually { active.state == NetworkPlaybackSession.State.PLAYING && active.positionUs > paused + 200_000 }
            report += "PASS pause freezes media position and resume advances it"
            active.seekTo(8_000_000)
            stage = "seek-forward"
            eventually { active.positionUs in 7_900_000..9_500_000 }
            active.seekTo(2_000_000)
            stage = "seek-backward"
            eventually { active.positionUs in 1_900_000..3_500_000 }
            report += "PASS forward and backward seek use the native A/V session"
            val steadyStart = active.positionUs
            stage = "steady"
            SystemClock.sleep(if (realSource >= 0) 20_000 else 3000)
            check(active.state == NetworkPlaybackSession.State.PLAYING) { active.lastError }
            if (realSource >= 0) {
                check(active.positionUs > steadyStart + 15_000_000) { "media position did not sustain progress" }
                report += "PASS real server playback advances at least 15 seconds in a 20-second observation"
            }
            runOnMainSync { host?.finish() }
            stage = "teardown"
            check(active.awaitClosed(15_000)) { "native teardown timeout" }
            val evidence = File(targetContext.filesDir, "evidence/network-native-sync.json")
            check(evidence.lastModified() >= evidenceBeganAt) { "stale native evidence" }
            val json = JSONObject(evidence.readText())
            check(json.getString("clock_source") == "audio_timestamp")
            val audio = json.getJSONObject("audio")
            check(audio.getBoolean("opened") && audio.getBoolean("started"))
            check(audio.getJSONObject("output").getLong("frames_rendered") > 0)
            check(json.getString("error").isEmpty())
            report += "PASS exit serializes native teardown; evidence confirms rendered audio and audio master clock"
        } catch (e: Throwable) {
            report += if (realSource >= 0) "FAIL stage=$stage type=${e.javaClass.simpleName} state=${network?.state} (details withheld for real-source privacy)" else "FAIL ${e.javaClass.simpleName}: ${e.message}"
            if (realSource >= 0) network?.lastError?.takeIf {
                it.isNotBlank() && !it.contains("://") && !it.contains("api_key", true) && !it.contains("token", true)
            }?.let { report += "player_reason=$it" }
        } finally {
            runOnMainSync { host?.finish() }
            network?.let { it.stop(); it.awaitClosed(15_000) }
            runCatching { proxy?.close() }
        }
        finish(if (report.any { it.startsWith("FAIL") }) Activity.RESULT_CANCELED else Activity.RESULT_OK,
            Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }

    private fun eventually(timeoutMs: Long = 8_000, predicate: () -> Boolean) {
        val deadline = SystemClock.uptimeMillis() + timeoutMs
        while (!predicate()) {
            check(SystemClock.uptimeMillis() < deadline) { "condition timeout" }
            SystemClock.sleep(50)
        }
    }
}
