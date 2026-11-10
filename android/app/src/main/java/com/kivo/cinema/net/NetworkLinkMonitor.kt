package com.kivo.cinema.net

import android.content.Context
import android.net.ConnectivityManager
import android.net.NetworkCapabilities

/**
 * 系统网络可达性查询（250 D3）。**唯一**碰 Android 网络 API 的地方。
 *
 * 为什么是"静态查询"而不是 `registerDefaultNetworkCallback`：
 *
 * 1. 回调要配对 `unregister`，要跟 Activity/Service 生命周期绑定，而 SMB 读发生在
 *    fd 回调线程 —— 多一个生命周期面，就多一类"注册了忘注销"的泄漏。
 * 2. 我们只在**读失败时**问一次"现在还有网吗"。查询是同步的、微秒级，
 *    调用频率 = 失败频率，不需要持续监听。
 * 3. 没有回调 = 没有状态 = 没有需要清理的东西。
 *
 * 它解决的是 P0-1 的一半：断网后 socket 不会立刻报错（TCP 要等重传超时），
 * 所以只能"读失败 → 问一句 → 发现没网 → 立刻告诉用户"，而不是干等 15 秒。
 * 另一半是把读超时从 15 s 降到 3 s（见 `smb/SmbReader`）。
 *
 * 保守取向：查不到就返回 true。宁可多试一次网络，也不要因为一次查询失败
 * 把还能读的播放判死 —— 误判"离线"会让用户看到莫名其妙的"网络已断开"。
 */
object NetworkLinkMonitor {

    fun isOnline(context: Context): Boolean {
        val manager = context.getSystemService(ConnectivityManager::class.java) ?: return true
        val network = manager.activeNetwork ?: return false
        val capabilities = manager.getNetworkCapabilities(network) ?: return true
        return capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
    }
}
