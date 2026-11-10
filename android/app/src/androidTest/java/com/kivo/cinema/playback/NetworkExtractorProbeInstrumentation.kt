package com.kivo.cinema.playback

import android.app.Activity
import android.app.Instrumentation
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Bundle
import android.os.SystemClock
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.net.HttpRangeFile
import com.kivo.cinema.sources.storage.SourceStore
import java.util.Timer
import java.util.TimerTask
import java.util.concurrent.atomic.AtomicInteger

/** Bounded, opt-in diagnostics; no credentials, addresses or media titles are emitted. */
class NetworkExtractorProbeInstrumentation : Instrumentation() {
    private var sourceIndex = 1
    private var itemIndex = 2
    private var audioPackets = 0
    private var videoPackets = 0

    override fun onCreate(arguments: Bundle?) {
        sourceIndex = arguments?.getString("source")?.toIntOrNull() ?: 1
        itemIndex = arguments?.getString("item")?.toIntOrNull() ?: 2
        audioPackets = arguments?.getString("audioPackets")?.toIntOrNull()?.coerceIn(0, 32) ?: 0
        videoPackets = arguments?.getString("videoPackets")?.toIntOrNull()?.coerceIn(0, 600) ?: 0
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        var success = false
        var stage = "login"
        val began = SystemClock.elapsedRealtime()
        val count = AtomicInteger()
        fun report(message: String) {
            sendStatus(0, Bundle().apply {
                putString("stream", "t=${SystemClock.elapsedRealtime() - began} $message\n")
            })
        }
        try {
            require(sourceIndex >= 0 && itemIndex in 0..19)
            val source = SourceStore(targetContext).read().getOrThrow().filterNot { it.hidden }[sourceIndex]
            val client = EmbyClient(source.address)
            val auth = client.login(source.username, source.password)
            val item = client.items(auth, start = itemIndex, limit = 1).items.first()
            report("source=${sourceIndex + 1} item=${itemIndex + 1} login=ok")
            HttpRangeFile(targetContext, client.streamUrl(auth, item.id)) { event ->
                if (count.incrementAndGet() <= 100) report(event)
            }.use { input ->
                stage = "prepare"
                input.prepare()
                input.openDescriptor().use { fd ->
                    val extractor = MediaExtractor()
                    val timer = Timer(true)
                    timer.schedule(object : TimerTask() {
                        override fun run() { report("deadline_cancel"); input.cancel() }
                    }, 45_000)
                    try {
                        stage = "extractor"
                        extractor.setDataSource(fd.fileDescriptor, 0, input.size)
                        val mimes = (0 until extractor.trackCount).map {
                            extractor.getTrackFormat(it).getString(MediaFormat.KEY_MIME) ?: "unknown"
                        }
                        report("tracks=${extractor.trackCount} mimes=${mimes.joinToString(",")} io_reason=${input.failureReason.ifBlank { "none" }}")
                        check(extractor.trackCount > 0)
                        if (videoPackets > 0) {
                            val video = mimes.indexOfFirst { it.startsWith("video/") }
                            check(video >= 0)
                            extractor.selectTrack(video)
                            val buffer = java.nio.ByteBuffer.allocate(16 * 1024 * 1024)
                            var previous = -1L
                            repeat(videoPackets) { index ->
                                stage = "video-read-$index"
                                val at = SystemClock.elapsedRealtime()
                                val pts = extractor.sampleTime
                                val size = extractor.readSampleData(buffer, 0)
                                check(size > 0)
                                check(extractor.advance())
                                report("video packet=$index pts_us=$pts delta_us=${pts - previous} bytes=$size elapsed_ms=${SystemClock.elapsedRealtime() - at}")
                                previous = pts
                            }
                            extractor.unselectTrack(video)
                        }
                        if (audioPackets > 0) {
                            val audio = mimes.indexOfFirst { it.startsWith("audio/") }
                            check(audio >= 0)
                            extractor.selectTrack(audio)
                            val buffer = java.nio.ByteBuffer.allocate(1024 * 1024)
                            fun packets(label: String) {
                                repeat(audioPackets) { index ->
                                    stage = "$label-read-$index"
                                    val at = SystemClock.elapsedRealtime()
                                    val pts = extractor.sampleTime
                                    val size = extractor.readSampleData(buffer, 0)
                                    report("$label packet=$index pts_us=$pts bytes=$size read_ms=${SystemClock.elapsedRealtime() - at}")
                                    check(size > 0)
                                    stage = "$label-advance-$index"
                                    val advanceAt = SystemClock.elapsedRealtime()
                                    check(extractor.advance())
                                    report("$label advance=$index elapsed_ms=${SystemClock.elapsedRealtime() - advanceAt}")
                                }
                            }
                            packets("audio")
                            stage = "audio-seek"
                            extractor.seekTo(6_250_000, MediaExtractor.SEEK_TO_PREVIOUS_SYNC)
                            report("audio seek_landing_us=${extractor.sampleTime}")
                            packets("audio-after-seek")
                        }
                        success = true
                    } finally {
                        timer.cancel()
                        extractor.release()
                    }
                }
            }
        } catch (e: Throwable) {
            report("FAIL stage=$stage type=${e.javaClass.simpleName}")
        }
        report("${if (success) "PASS" else "FAIL"} bounded extractor probe events=${count.get()}")
        finish(if (success) Activity.RESULT_OK else Activity.RESULT_CANCELED, Bundle())
    }
}
