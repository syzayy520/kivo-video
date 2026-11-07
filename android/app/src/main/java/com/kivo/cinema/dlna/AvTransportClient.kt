package com.kivo.cinema.dlna

import org.xmlpull.v1.XmlPullParser
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL

/**
 * AVTransport SOAP 控制（230 设计稿 D2/D3）：描述解析 + Set/Play/Pause/Stop/Seek。
 *
 * 全同步阻塞，调用方自备线程。零第三方（XmlPullParser 平台内置）。
 * 失败抛 IllegalStateException（reason 带上下文，G9 用户可见）。
 */
class AvTransportClient {

    /** 解析设备描述 XML → friendlyName + AVTransport controlURL（相对则按 LOCATION resolve）。 */
    fun resolve(device: DlnaDevice): DlnaDevice {
        val conn = URL(device.location).openConnection() as HttpURLConnection
        conn.connectTimeout = 5000
        conn.readTimeout = 10000
        val code = conn.responseCode
        val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
            ?.let { InputStreamReader(it, Charsets.UTF_8).use { r -> r.readText() } } ?: ""
        if (code !in 200..299) {
            throw IllegalStateException("描述获取失败 HTTP $code（${device.location}）")
        }

        var friendly = device.friendlyName
        var controlUrl = ""
        var inAvTransport = false
        var tag: String? = null
        val parser = android.util.Xml.newPullParser()
        parser.setInput(body.reader())
        var event = parser.eventType
        while (event != XmlPullParser.END_DOCUMENT) {
            when (event) {
                XmlPullParser.START_TAG -> {
                    tag = parser.name
                    when (parser.name) {
                        "service" -> inAvTransport = false
                        "friendlyName" -> Unit
                    }
                }
                XmlPullParser.TEXT -> {
                    val t = parser.text?.trim() ?: ""
                    if (t.isEmpty()) Unit   // 标签间空白也是 TEXT 事件——跳过，否则会重置状态
                    else when (tag) {
                        "friendlyName" -> if (friendly.isEmpty()) friendly = t
                        "serviceType" -> inAvTransport =
                            t == "urn:schemas-upnp-org:service:AVTransport:1"
                        "controlURL" -> if (inAvTransport && controlUrl.isEmpty()) {
                            controlUrl = t
                        }
                    }
                }
            }
            event = parser.next()
        }
        if (friendly.isEmpty()) friendly = device.usn.take(24)
        if (controlUrl.isEmpty()) {
            throw IllegalStateException("描述里没有 AVTransport/controlURL（${device.location}）")
        }
        val abs = if (controlUrl.startsWith("http://") || controlUrl.startsWith("https://")) {
            controlUrl
        } else {
            val base = URL(device.location)
            val path = base.path.substringBeforeLast('/')
            URL(base, if (controlUrl.startsWith("/")) controlUrl else "$path/$controlUrl").toString()
        }
        return device.copy(friendlyName = friendly, controlUrl = abs)
    }

    private fun soap(device: DlnaDevice, action: String, args: String): String {
        val conn = URL(device.controlUrl).openConnection() as HttpURLConnection
        conn.connectTimeout = 5000
        conn.readTimeout = 10000
        conn.requestMethod = "POST"
        conn.doOutput = true
        conn.setRequestProperty("Content-Type", "text/xml; charset=\"utf-8\"")
        conn.setRequestProperty("SOAPACTION",
            "\"urn:schemas-upnp-org:service:AVTransport:1#$action\"")
        val envelope = "<?xml version=\"1.0\" encoding=\"utf-8\"?>" +
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" " +
            "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>" +
            "<u:$action xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">" +
            args + "</u:$action></s:Body></s:Envelope>"
        conn.outputStream.use { it.write(envelope.toByteArray(Charsets.UTF_8)) }
        val code = conn.responseCode
        val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
            ?.let { InputStreamReader(it, Charsets.UTF_8).use { r -> r.readText() } } ?: ""
        if (code !in 200..299) {
            throw IllegalStateException("$action HTTP $code：${body.take(160)}")
        }
        if (body.contains("<u:$action", ignoreCase = true)) return body
        if (body.contains(":Fault") || body.contains("UPnPError")) {
            val desc = Regex("<errorDescription>([^<]*)").find(body)?.groupValues?.get(1) ?: ""
            throw IllegalStateException("$action UPnP Fault：$desc")
        }
        return body
    }

    private val instanceArgs = "<InstanceID>0</InstanceID>"

    fun setUri(device: DlnaDevice, uri: String, title: String) {
        val meta = "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " +
            "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" " +
            "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">" +
            "<item id=\"kivo\" parentID=\"0\" restricted=\"1\">" +
            "<dc:title>${title.take(120)}</dc:title>" +
            "<upnp:class>object.item.videoItem</upnp:class></item></DIDL-Lite>"
        soap(device, "SetAVTransportURI", instanceArgs +
            "<CurrentURI>$uri</CurrentURI>" +
            "<CurrentURIMetadata>$meta</CurrentURIMetadata>")
    }

    fun play(device: DlnaDevice) {
        soap(device, "Play", instanceArgs + "<Speed>1</Speed>")
    }

    fun pause(device: DlnaDevice) {
        soap(device, "Pause", instanceArgs)
    }

    fun stop(device: DlnaDevice) {
        soap(device, "Stop", instanceArgs)
    }

    fun seek(device: DlnaDevice, positionMs: Long) {
        val s = (positionMs / 1000).coerceAtLeast(0)
        val target = "%02d:%02d:%02d".format(s / 3600, (s % 3600) / 60, s % 60)
        soap(device, "Seek", instanceArgs + "<Unit>REL_TIME</Unit><Target>$target</Target>")
    }
}
