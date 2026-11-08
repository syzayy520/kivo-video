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
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }
    override fun onStart() {
        val report = mutableListOf<String>()
        var host: PlayerComposeActivity? = null
        var network: NetworkPlaybackSession? = null
        var proxy: LocalRangeProxy? = null
        try {
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

            val monitor = addMonitor(PlayerComposeActivity::class.java.name, null, false)
            // Shell launch also works when OEM background-activity policy blocks Context.startActivity.
            uiAutomation.executeShellCommand("am start -W -n com.kivo.cinema/.ui.compose.PlayerComposeActivity --es kivo.playurl $url --es kivo.label synthetic")
                .use { fd -> android.os.ParcelFileDescriptor.AutoCloseInputStream(fd).readBytes() }
            host = waitForMonitorWithTimeout(monitor, 15_000) as? PlayerComposeActivity
                ?: error("production host timeout")
            removeMonitor(monitor)
            val field = PlayerComposeActivity::class.java.getDeclaredField("smbNetSession").apply { isAccessible = true }
            eventually(25_000) {
                runOnMainSync { network = field.get(host) as? NetworkPlaybackSession }
                network?.state == NetworkPlaybackSession.State.PLAYING || network?.state == NetworkPlaybackSession.State.FAILED
            }
            val active = network!!
            check(active.state == NetworkPlaybackSession.State.PLAYING) { active.lastError }
            check(active.hasAudio)
            check(!active.codecName.startsWith("OMX.google") && !active.codecName.startsWith("c2.android"))
            eventually { active.positionUs > 1_000_000 }
            report += "PASS production network host starts native hardware video and attaches audio"
            active.setPaused(true)
            eventually { active.state == NetworkPlaybackSession.State.PAUSED }
            val paused = active.positionUs
            SystemClock.sleep(600)
            check(kotlin.math.abs(active.positionUs - paused) < 100_000)
            active.setPaused(false)
            eventually { active.state == NetworkPlaybackSession.State.PLAYING && active.positionUs > paused + 200_000 }
            report += "PASS pause freezes media position and resume advances it"
            active.seekTo(8_000_000)
            eventually { active.positionUs in 7_900_000..9_500_000 }
            active.seekTo(2_000_000)
            eventually { active.positionUs in 1_900_000..3_500_000 }
            report += "PASS forward and backward seek use the native A/V session"
            SystemClock.sleep(3000)
            check(active.state == NetworkPlaybackSession.State.PLAYING) { active.lastError }
            runOnMainSync { host?.finish() }
            check(active.awaitClosed(15_000)) { "native teardown timeout" }
            val evidence = File(targetContext.filesDir, "evidence/network-native-sync.json")
            val json = JSONObject(evidence.readText())
            check(json.getString("clock_source") == "audio_timestamp")
            val audio = json.getJSONObject("audio")
            check(audio.getBoolean("opened") && audio.getBoolean("started"))
            check(audio.getJSONObject("output").getLong("frames_rendered") > 0)
            check(json.getString("error").isEmpty())
            report += "PASS exit serializes native teardown; evidence confirms rendered audio and audio master clock"
        } catch (e: Throwable) {
            report += "FAIL ${e.javaClass.simpleName}: ${e.message}"
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
