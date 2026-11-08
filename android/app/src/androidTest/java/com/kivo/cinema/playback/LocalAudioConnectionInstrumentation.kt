package com.kivo.cinema.playback

import android.app.Activity
import android.app.Instrumentation
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import com.kivo.cinema.bridge.MediaTrackProbe
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import java.io.File
import org.json.JSONObject

/** Exercises the production Compose host with public synthetic fixtures. */
class LocalAudioConnectionInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }
    override fun onStart() {
        val report = mutableListOf<String>()
        var host: PlayerComposeActivity? = null
        val evidence = File(targetContext.filesDir, "evidence/local-audio-connection.json")
        try {
            val media = File(targetContext.getExternalFilesDir(null), "media/local-audio-connection.mp4")
            check(MediaTrackProbe.inspectAudioTrack(media).getOrThrow())
            check(MediaTrackProbe.inspectAudioTrack(File(media.parentFile, "local-video-only.mp4")).getOrThrow() == false)
            check(MediaTrackProbe.inspectAudioTrack(File(media.parentFile, "absent-audio-fixture.mp4")).isFailure)
            report += "PASS strict probe distinguishes audio, no audio, and inaccessible source"
            val monitor = addMonitor(PlayerComposeActivity::class.java.name, null, false)
            targetContext.startActivity(Intent(targetContext, PlayerComposeActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK).putExtra("kivo.media", media.absolutePath))
            host = waitForMonitorWithTimeout(monitor, 10_000) as? PlayerComposeActivity ?: error("host timeout")
            removeMonitor(monitor)
            val field = PlayerComposeActivity::class.java.getDeclaredField("playbackSession").apply { isAccessible = true }
            var session: PlaybackSession? = null
            val until = SystemClock.uptimeMillis() + 12_000
            while (session?.isRunning() != true) {
                check(SystemClock.uptimeMillis() < until) { "playback start timeout" }
                runOnMainSync { session = field.get(host) as? PlaybackSession }
                SystemClock.sleep(50)
            }
            val active = session!!
            check(active.audioAttached)
            SystemClock.sleep(2500)
            check(active.positionUs() > 1_000_000)
            report += "PASS production Compose local session attaches native audio and advances media position"
            runOnMainSync {
                active.stop()
                check(active.writeSyncStats(evidence.absolutePath) == 0)
            }
            val json = JSONObject(evidence.readText())
            val audio = json.getJSONObject("audio")
            check(audio.getBoolean("opened") && audio.getBoolean("started"))
            check(audio.getJSONObject("output").getLong("frames_rendered") > 0)
            check(json.getString("clock_source") == "audio_timestamp")
            check(json.getString("error").isEmpty())
            report += "PASS native evidence confirms audio opened/started, rendered frames, audio master clock, and no session error"
        } catch (e: Throwable) {
            report += "FAIL ${e.javaClass.simpleName}: ${e.message}"
        } finally {
            runOnMainSync {
                host?.finish()
                targetContext.stopService(Intent(targetContext, PlaybackService::class.java))
            }
        }
        finish(if (report.any { it.startsWith("FAIL") }) Activity.RESULT_CANCELED else Activity.RESULT_OK,
            Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }
}
