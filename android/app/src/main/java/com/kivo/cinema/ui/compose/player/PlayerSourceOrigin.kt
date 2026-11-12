package com.kivo.cinema.ui.compose.player

import android.content.Context
import android.content.Intent
import com.kivo.cinema.sources.storage.SourceStore
import java.net.URI

/**
 * 源卡的"来自哪里"（v3.1f；REX 0169 源卡显示的是 nyamedia 服务器名，不是文件名）。
 *
 * 【为什么单独一个文件】来源名要跨三种协议（SMB 路由器/NAS、Emby、普通 URL），
 * 还要查源记录拿用户起的名字。混在面板装配里会让"取数"和"来源推导"缠在一起。
 *
 * [from] 只读 intent（主线程安全）；[resolve] 查磁盘，**只在 IO 线程调**。
 */
internal data class PlayerSourceOrigin(
    val sourceId: String?,
    val explicitName: String?,
    val smbShare: String?,
    val urlHost: String?,
) {
    /** 优先级：显式服务器名 > 源记录名 > SMB 共享名 > URL 主机 > 未知。不编造来源。 */
    fun resolve(context: Context): String {
        explicitName?.takeIf { it.isNotBlank() }?.let { return it }
        if (!sourceId.isNullOrBlank()) {
            runCatching {
                SourceStore(context).read().getOrThrow().firstOrNull { it.id == sourceId }?.name
            }.getOrNull()?.takeIf { it.isNotBlank() }?.let { return it }
        }
        smbShare?.takeIf { it.isNotBlank() }?.let { return it }
        urlHost?.takeIf { it.isNotBlank() }?.let { return it }
        return "未知来源"
    }

    companion object {
        fun from(intent: Intent): PlayerSourceOrigin = PlayerSourceOrigin(
            sourceId = intent.getStringExtra("kivo.sourceId"),
            explicitName = intent.getStringExtra("kivo.sourceName"),
            smbShare = intent.getStringExtra("kivo.smbShare"),
            urlHost = intent.getStringExtra("kivo.playurl")?.let {
                runCatching { URI(it).host }.getOrNull()
            },
        )
    }
}