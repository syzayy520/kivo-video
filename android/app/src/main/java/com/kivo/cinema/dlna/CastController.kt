package com.kivo.cinema.dlna

import android.content.Context
import android.net.wifi.WifiManager
import android.util.Log
import com.kivo.cinema.net.LocalRangeProxy
import java.net.NetworkInterface

/**
 * 投屏会话编排（230 设计稿 D4/D6）：扫描 → 选中 → URL 策略 → Set+Play → Stop。
 *
 * URL 策略：ByteSource → LAN 代理（绑 0.0.0.0，渲染器来取）；http(s) URL → 直传。
 * 单一职责：会话编排——发现/协议在 SsdpClient/AvTransportClient，UI 在 PlayerControls。
 */
class CastController(private val context: Context) {

    companion object {
        private const val TAG = "KivoDlna"
    }

    private val avt = AvTransportClient()
    private var proxy: LocalRangeProxy? = null
    private var multicastLock: WifiManager.MulticastLock? = null

    /** 当前投屏目标（null = 未投屏）。UI 观察。 */
    var castingTo: DlnaDevice? = null
        private set

    /** 扫描渲染器（阻塞，调用方备线程）。返回已解析 friendlyName/controlUrl 的设备。 */
    fun scan(trace: ((String) -> Unit)? = null): List<DlnaDevice> {
        acquireMulticastLock()
        try {
            val raw = SsdpClient().scan(trace = trace)
            val resolved = mutableListOf<DlnaDevice>()
            for (d in raw) {
                try {
                    resolved += avt.resolve(d)
                } catch (e: Exception) {
                    Log.w(TAG, "resolve failed ${d.location}", e)
                    trace?.invoke("resolve fail ${d.location}: " +
                        "${e.javaClass.simpleName} ${e.message?.take(120)}")
                }
            }
            return resolved
        } finally {
            releaseMulticastLock()
        }
    }

    /** 投本地/远端 ByteSource（LAN 代理 URL）。阻塞，调用方备线程。 */
    fun castSource(device: DlnaDevice, source: com.kivo.cinema.smb.SmbSource.ByteSource,
                   title: String): Int {
        val port = startLanProxy(source)
        return castUrl(device, "http://${wifiIp()}:$port/kivo", title)
    }

    /** 投 http(s) URL（渲染端自己取流）。阻塞，调用方备线程。 */
    fun castUrl(device: DlnaDevice, url: String, title: String): Int {
        return try {
            avt.setUri(device, url, title)
            avt.play(device)
            castingTo = device
            0
        } catch (e: Exception) {
            Log.w(TAG, "cast failed", e)
            castingTo = null
            1
        }
    }

    /** 停止投屏（Stop + 关代理）。阻塞，调用方备线程。 */
    fun stop(): Int {
        val d = castingTo
        var rc = 0
        if (d != null) {
            rc = try {
                avt.stop(d)
                0
            } catch (e: Exception) {
                Log.w(TAG, "dlna stop failed", e)
                1
            }
        }
        castingTo = null
        runCatching { proxy?.close() }
        proxy = null
        return rc
    }

    private fun startLanProxy(source: com.kivo.cinema.smb.SmbSource.ByteSource): Int {
        runCatching { proxy?.close() }
        val p = LocalRangeProxy(source, bindAddress = "0.0.0.0")
        val port = p.start()
        proxy = p
        return port
    }

    private fun wifiIp(): String {
        val it = NetworkInterface.getNetworkInterfaces()
        while (it.hasMoreElements()) {
            val nif = it.nextElement()
            if (!nif.isUp || nif.isLoopback || nif.isVirtual) continue
            for (addr in nif.inetAddresses) {
                if (!addr.isLoopbackAddress && addr.address.size == 4) {
                    return addr.hostAddress ?: ""
                }
            }
        }
        return ""
    }

    private fun acquireMulticastLock() {
        if (multicastLock?.isHeld == true) return
        val wifi = context.applicationContext
            .getSystemService(Context.WIFI_SERVICE) as WifiManager
        multicastLock = wifi.createMulticastLock("kivo-dlna").apply {
            setReferenceCounted(false)
            acquire()
        }
    }

    private fun releaseMulticastLock() {
        multicastLock?.let { if (it.isHeld) it.release() }
    }
}
