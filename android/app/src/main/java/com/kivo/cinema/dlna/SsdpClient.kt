package com.kivo.cinema.dlna

import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.NetworkInterface
import java.net.SocketTimeoutException

/**
 * SSDP 发现（230 设计稿 D1）：M-SEARCH 主动扫为主，同窗口顺带收 ssdp:alive 为副。
 *
 * 调用方负责持有 WifiManager.MulticastLock（否则 alive 收不到；M-SEARCH 应答是单播不受限）。
 * 全同步阻塞，调用方自备线程。零第三方。
 */
class SsdpClient {

    private val msearch = (
        "M-SEARCH * HTTP/1.1\r\n" +
        "HOST: 239.255.255.250:1900\r\n" +
        "MAN: \"ssdp:discover\"\r\n" +
        "MX: 3\r\n" +
        "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n" +
        "\r\n"
    ).toByteArray()

    /**
     * 扫描 [windowMs] 毫秒；返回去重后的渲染器候选（未解析 friendlyName）。
     * 只收 Location 非空的应答/广播；usn 去重。
     */
    fun scan(windowMs: Long = 4000, trace: ((String) -> Unit)? = null):
        List<DlnaDevice> {
        val wifiNi = NetworkInterface.getNetworkInterfaces().asSequence()
            .firstOrNull { it.name.startsWith("wlan") && it.isUp && !it.isLoopback }
        trace?.invoke("wifiNi=${wifiNi?.name} addr=${wifiIpv4(wifiNi)}")
        val found = LinkedHashMap<String, DlnaDevice>()
        java.net.MulticastSocket().use { sock ->
            sock.broadcast = true
            sock.soTimeout = 500
            val group = InetAddress.getByName("239.255.255.250")
            if (wifiNi != null) {
                sock.networkInterface = wifiNi   // M-SEARCH 出口绑 wlan
                try {
                    sock.joinGroup(java.net.InetSocketAddress(group, 1900), wifiNi)
                } catch (e: Exception) {
                    trace?.invoke("join failed: ${e.message}")
                }
            }
            // M-SEARCH 发三遍（MX 窗口内多设备应答概率更高）
            repeat(3) {
                sock.send(DatagramPacket(msearch, msearch.size, group, 1900))
                Thread.sleep(120)
            }
            // 单播扫段兜底（2026-11-08 真机实测：MIUI 组播发送被驱动丢弃，
            // 小爱音箱对单播 M-SEARCH 应答正常——nc 实证）。/24 逐主机打
            // 1900（标准 SSDP）+ 1901（kivo 测试渲染器约定端口，绕 Windows WFP）。
            val localIp = wifiIpv4(wifiNi)
            if (localIp != null) {
                val prefix = localIp.substringBeforeLast('.')
                Thread {
                    for (i in 1..254) {
                        try {
                            val t = InetAddress.getByName("$prefix.$i")
                            sock.send(DatagramPacket(msearch, msearch.size, t, 1900))
                            sock.send(DatagramPacket(msearch, msearch.size, t, 1901))
                        } catch (e: Exception) {
                            // 单台目标失败不中断扫段
                        }
                    }
                }.start()
            }
            val deadline = System.currentTimeMillis() + windowMs
            val buf = ByteArray(2048)
            var rxCount = 0
            while (System.currentTimeMillis() < deadline) {
                val pkt = DatagramPacket(buf, buf.size)
                try {
                    sock.receive(pkt)
                    rxCount++
                } catch (e: SocketTimeoutException) {
                    continue
                }
                val text = String(buf, 0, pkt.length, Charsets.UTF_8)
                trace?.invoke("rx#${rxCount} from ${pkt.address.hostAddress}: " +
                    text.split('\n').firstOrNull()?.trim()?.take(40))
                val loc = Regex("(?i)LOCATION:\\s*(\\S+)").find(text)?.groupValues?.get(1)
                    ?: continue
                val usn = Regex("(?i)USN:\\s*(\\S+)").find(text)?.groupValues?.get(1) ?: loc
                if (found.containsKey(usn)) continue
                found[usn] = DlnaDevice(usn = usn, location = loc)
            }
            trace?.invoke("scan end rx=$rxCount found=${found.size}")
        }
        return found.values.toList()
    }

    private fun wifiIpv4(ni: NetworkInterface?): String? {
        if (ni == null) return null
        for (addr in ni.inetAddresses) {
            if (!addr.isLoopbackAddress && addr.address.size == 4) return addr.hostAddress
        }
        return null
    }
}
