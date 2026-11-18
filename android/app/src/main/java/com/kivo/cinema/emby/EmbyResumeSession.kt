package com.kivo.cinema.emby

import java.util.concurrent.ConcurrentHashMap

/**
 * 首页 Emby 续播的**会话缓存**：[EmbyHomeResume] 取数时把已登录会话记下来，
 * 海报组件按 sourceId 复用，避免每张图都重新 `login`（一次 login 是一次网络往返）。
 *
 * 单一职责：只做 "sourceId → 已登录会话" 的内存缓存，不取数、不渲染、不落盘。
 * 凭据**只在本进程内存**：不进 [com.kivo.cinema.library.HomeEntry]、不写磁盘。
 */
object EmbyResumeSession {

    private class Session(val client: EmbyClient, val auth: EmbyClient.Auth)

    private val sessions = ConcurrentHashMap<String, Session>()

    fun remember(sourceId: String, client: EmbyClient, auth: EmbyClient.Auth) {
        sessions[sourceId] = Session(client, auth)
    }

    /** 拉海报字节；会话缺失或任何失败返回 null（组件显示中性占位，不崩不白屏）。 */
    fun fetchBytes(sourceId: String?, url: String): ByteArray? {
        val s = sourceId?.let { sessions[it] } ?: return null
        return runCatching { s.client.fetchBytes(url, s.auth.token) }.getOrNull()
    }
}
