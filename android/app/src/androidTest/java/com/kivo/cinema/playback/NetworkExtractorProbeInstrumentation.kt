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

    override fun onCreate(arguments: Bundle?) {
        sourceIndex = arguments?.getString("source")?.toIntOrNull() ?: 1
        itemIndex = arguments?.getString("item")?.toIntOrNull() ?: 2
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
