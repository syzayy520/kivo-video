package com.kivo.cinema.tmdb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import java.net.InetAddress
import java.net.HttpURLConnection
import java.net.Proxy
import java.net.URL
import org.json.JSONObject

/** Explicit real-device diagnostic, not an offline test or a network availability assertion. */
class TmdbConnectivityInstrumentation : Instrumentation() {
    override fun onCreate(arguments: Bundle?) { super.onCreate(arguments); start() }

    override fun onStart() {
        val report = mutableListOf<String>()
        for (host in listOf("api.themoviedb.org", "api.tmdb.org", "image.tmdb.org", "media.themoviedb.org")) {
            val dns = runCatching { InetAddress.getAllByName(host).joinToString { it.hostAddress.orEmpty() } }
                .getOrElse { it.javaClass.simpleName }
            report += "$host DNS=$dns"
            val connection = URL("https://$host/" + if (host.startsWith("api.")) "3/configuration" else "t/p/w92/invalid.jpg")
                .openConnection(Proxy.NO_PROXY) as HttpURLConnection
            val started = System.nanoTime()
            val result = try {
                connection.connectTimeout = 6000
                connection.readTimeout = 6000
                "HTTP=${connection.responseCode}"
            } catch (e: Exception) { e.javaClass.simpleName }
            finally { connection.disconnect() }
            report += "$host direct $result elapsedMs=${(System.nanoTime() - started) / 1_000_000}"
        }
        val key = TmdbClient(targetContext).apiKey()
        if (key.isNotBlank()) {
            val api = URL("https://api.tmdb.org/3/movie/now_playing?api_key=$key&language=zh-CN")
                .openConnection(Proxy.NO_PROXY) as HttpURLConnection
            try {
                api.connectTimeout = 6000; api.readTimeout = 6000
                val status = api.responseCode
                report += "API authenticated HTTP=$status"
                if (status == 200) {
                    val items = JSONObject(api.inputStream.bufferedReader().use { it.readText() }).getJSONArray("results")
                    report += "API resultCount=${items.length()}"
                    val path = items.optJSONObject(0)?.optString("poster_path").orEmpty()
                    if (path.isNotEmpty()) {
                        for (imageHost in listOf("image.tmdb.org", "media.themoviedb.org")) {
                        val image = URL("https://$imageHost/t/p/w92$path").openConnection(Proxy.NO_PROXY) as HttpURLConnection
                        try {
                            image.connectTimeout = 6000; image.readTimeout = 6000
                            report += "$imageHost actual poster HTTP=${image.responseCode}"
                            if (image.responseCode == 200) {
                                val bitmap = image.inputStream.use { android.graphics.BitmapFactory.decodeStream(it) }
                                report += "$imageHost actual poster decoded=${bitmap != null}"
                                bitmap?.recycle()
                            }
                        } catch (e: Exception) { report += "$imageHost actual poster ${e.javaClass.simpleName}" }
                        finally { image.disconnect() }
                        }
                        TmdbClient.setShared(TmdbClient(targetContext))
                        // w154 is not used by the home image cache; exercise the production loader.
                        val loaded = com.kivo.cinema.ui.design.TmdbImageCache.load("https://image.tmdb.org/t/p/w154$path")
                        report += "Production poster loader decoded=${loaded != null}"
                    }
                }
            } catch (e: Exception) { report += "API authenticated ${e.javaClass.simpleName}" }
            finally { api.disconnect() }
        }
        finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "\n${report.joinToString("\n")}\n") })
    }
}
