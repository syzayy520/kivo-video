package com.kivo.cinema.smb.discovery

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Handler
import android.os.Looper

/**
 * 局域网 SMB 发现（243 D8）：系统 NSD（mDNS `_smb._tcp`），不引第三方库。
 *
 * 实测（2026-11-09，本机网络）：只有 NAS `N1`（192.168.5.197）广播 `_smb._tcp`；
 * 小米路由（192.168.5.1）**不广播** —— 所以发现是"帮用户省打字"，不是"唯一入口"，
 * 手工地址永远保留。Rex 的"可用地址"里也只有 N1，行为一致。
 *
 * 生命周期：[start] 后 8 秒自动停（[stop] 可提前）；回调都在主线程。
 * 注意 NSD 的 resolve 同一时刻只能有一个，这里用队列串行化。
 */
class SmbDiscovery(context: Context) {

    data class Found(val label: String, val host: String, val port: Int)

    private val manager = context.applicationContext
        .getSystemService(Context.NSD_SERVICE) as NsdManager
    private val main = Handler(Looper.getMainLooper())
    private val pending = ArrayDeque<NsdServiceInfo>()
    private var resolving = false
    private var discovery: NsdManager.DiscoveryListener? = null
    private var stopped = false

    /**
     * 开始浏览。[onFound] 在主线程回调。
     *
     * `NsdManager` 的部分 ROM 实现要求调用方带 Looper，所以这里统一转到主线程，
     * 调用方（Compose / 测试线程）不必关心。
     */
    fun start(onFound: (Found) -> Unit) {
        main.post { startOnMain(onFound) }
    }

    private fun startOnMain(onFound: (Found) -> Unit) {
        stop()
        stopped = false
        val listener = object : NsdManager.DiscoveryListener {
            override fun onStartDiscoveryFailed(serviceType: String?, errorCode: Int) = stop()
            override fun onStopDiscoveryFailed(serviceType: String?, errorCode: Int) = stop()
            override fun onDiscoveryStarted(serviceType: String?) = Unit
            override fun onDiscoveryStopped(serviceType: String?) = Unit

            override fun onServiceFound(info: NsdServiceInfo?) {
                if (info == null || stopped) return
                synchronized(pending) { pending.addLast(info) }
                pump(onFound)
            }

            override fun onServiceLost(info: NsdServiceInfo?) = Unit
        }
        discovery = listener
        runCatching { manager.discoverServices(TYPE, NsdManager.PROTOCOL_DNS_SD, listener) }
            .onFailure { stopped = true }
        main.postDelayed({ stop() }, TIMEOUT_MS)
    }

    fun stop() {
        stopped = true
        discovery?.let { runCatching { manager.stopServiceDiscovery(it) } }
        discovery = null
        synchronized(pending) { pending.clear() }
        resolving = false
    }

    private fun pump(onFound: (Found) -> Unit) {
        if (stopped || resolving) return
        val next = synchronized(pending) { pending.removeFirstOrNull() } ?: return
        resolving = true
        val resolveListener = object : NsdManager.ResolveListener {
            override fun onResolveFailed(info: NsdServiceInfo?, errorCode: Int) {
                resolving = false
                pump(onFound)
            }

            override fun onServiceResolved(info: NsdServiceInfo?) {
                resolving = false
                val host = info?.host?.hostAddress
                if (!host.isNullOrBlank() && !stopped) {
                    val found = Found(
                        label = info.serviceName.removeSuffix(".$TYPE").removeSuffix("."),
                        host = host.substringBefore('%'),   // IPv6 链路本地地址带 %wlan0，去掉作用域
                        port = info.port.coerceAtLeast(445),
                    )
                    main.post { onFound(found) }
                }
                pump(onFound)
            }
        }
        runCatching { manager.resolveService(next, resolveListener) }
            .onFailure { resolving = false }
    }

    companion object {
        private const val TYPE = "_smb._tcp"
        private const val TIMEOUT_MS = 8_000L
    }
}
