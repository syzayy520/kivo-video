package com.kivo.cinema.playback

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.sources.storage.SourceStore
import java.net.HttpURLConnection
import java.net.URL

/** Opt-in on-device diagnostic. Credentials and media identifiers never leave memory. */
class RealEmbyRangeInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val report = mutableListOf<String>()
        try {
            val sources = SourceStore(targetContext).read().getOrThrow().filterNot { it.hidden }
            check(sources.isNotEmpty())
            sources.forEachIndexed { sourceIndex, source ->
                val label = "source-${sourceIndex + 1}"
                try {
                    val client = EmbyClient(source.address)
                    val auth = client.login(source.username, source.password)
                    val items = client.items(auth, limit = 2).items
                    report += "$label login=ok items=${items.size}"
                    items.forEachIndexed { index, item ->
                        val began = SystemClock.elapsedRealtime()
                        val c = URL(client.streamUrl(auth, item.id)).openConnection() as HttpURLConnection
                        c.connectTimeout = 15_000
                        c.readTimeout = 30_000
                        c.setRequestProperty("Range", "bytes=0-4095")
                        c.setRequestProperty("Accept-Encoding", "identity")
                        try {
                            val code = c.responseCode
                            val headersMs = SystemClock.elapsedRealtime() - began
                            val validRange = Regex("bytes 0-\\d+/\\d+").matches(c.getHeaderField("Content-Range") ?: "")
                            val bytes = if (code == 206) c.inputStream.use { stream ->
                                val data = ByteArray(4096)
                                var count = 0
                                while (count < data.size) {
                                    val n = stream.read(data, count, data.size - count)
                                    if (n < 0) break
                                    count += n
                                }
                                count
                            } else 0
                            report += "$label item-${index + 1} http=$code valid_range=$validRange bytes=$bytes headers_ms=$headersMs total_ms=${SystemClock.elapsedRealtime() - began}"
                        } catch (e: Exception) {
                            report += "$label item-${index + 1} failure=${e.javaClass.simpleName} elapsed_ms=${SystemClock.elapsedRealtime() - began}"
                        } finally { c.disconnect() }
                    }
                } catch (e: Exception) { report += "$label failure=${e.javaClass.simpleName}" }
            }
        } catch (e: Exception) { report += "diagnostic_failure=${e.javaClass.simpleName}" }
        finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }
}
