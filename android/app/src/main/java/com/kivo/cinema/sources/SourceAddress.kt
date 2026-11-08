package com.kivo.cinema.sources

import java.net.URI
import java.util.Locale

/** A service root, never a playback URL. Credentials and query tokens are forbidden. */
object SourceAddress {
    fun normalize(input: String): String {
        val raw = input.trim()
        require(raw.isNotEmpty()) { "请输入服务器地址" }
        val uri = try { URI(if (raw.contains("://")) raw else "https://$raw") }
            catch (_: Exception) { throw IllegalArgumentException("服务器地址格式不正确") }
        val scheme = uri.scheme?.lowercase(Locale.ROOT)
        require(scheme == "http" || scheme == "https") { "仅支持 HTTP 或 HTTPS 地址" }
        require(!uri.host.isNullOrBlank() && uri.rawUserInfo == null &&
            uri.rawQuery == null && uri.rawFragment == null) { "地址不能包含账号、查询参数或片段" }
        require(uri.port == -1 || uri.port in 1..65535) { "端口应为 1–65535" }
        val host = uri.host.lowercase(Locale.ROOT)
        val port = uri.port.takeUnless { it == -1 || (scheme == "https" && it == 443) ||
            (scheme == "http" && it == 80) }
        val path = uri.rawPath.orEmpty().trimEnd('/')
        require(!path.contains(Regex("(?i)%2e|(?:^|/)\\.\\.?(?:/|$)"))) { "地址路径不能包含相对跳转" }
        return "$scheme://$host${port?.let { ":$it" }.orEmpty()}$path"
    }
}
