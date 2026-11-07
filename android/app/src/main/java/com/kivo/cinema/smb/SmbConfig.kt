package com.kivo.cinema.smb

import org.json.JSONObject

/**
 * SMB 连接描述（220 设计稿 D1/D5）：纯数据，无协议逻辑。
 * 字段对齐 Rex 表单键（rex-package Library.json `library.form.*`）：
 * host/端口/路径/用户名/密码；domain 可选；名称自动获取（浏览首层后回填）。
 */
data class SmbConfig(
    val host: String,
    val port: Int = 445,
    val share: String,
    val path: String = "",      // 共享内路径，如 "电影"；空 = 共享根
    val domain: String = "",
    val username: String = "",  // 空 = guest
    val password: String = "",
) {
    fun toJson(): String {
        val o = JSONObject()
        o.put("host", host)
        o.put("port", port)
        o.put("share", share)
        o.put("path", path)
        o.put("domain", domain)
        o.put("username", username)
        o.put("password", password)
        return o.toString()
    }

    companion object {
        fun fromJson(s: String): SmbConfig? = runCatching {
            val o = JSONObject(s)
            SmbConfig(
                host = o.getString("host"),
                port = o.optInt("port", 445),
                share = o.getString("share"),
                path = o.optString("path", ""),
                domain = o.optString("domain", ""),
                username = o.optString("username", ""),
                password = o.optString("password", ""),
            )
        }.getOrNull()
    }
}
