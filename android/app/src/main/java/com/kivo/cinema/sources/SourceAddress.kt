package com.kivo.cinema.sources

import java.net.URI
import java.util.Locale

/**
 * 源地址规范化（243 D5）。一个地址只描述"服务根"，不携带账号、查询参数或片段。
 *
 * 支持两种形态：
 * - `emby`：`http(s)://host[:port][/path]`（原有语义，默认补 https）
 * - `smb`：`smb://host[:port][/share][/path]`（243 新增；大小写与空格原样保留，
 *   因为 SMB 的共享名和文件名都是大小写敏感/可含空格的）
 */
object SourceAddress {

    fun normalize(input: String, protocol: String = SourceRecord.PROTOCOL_EMBY): String =
        when (protocol) {
            SourceRecord.PROTOCOL_SMB -> normalizeSmb(input)
            else -> normalizeHttp(input)
        }

    private fun normalizeHttp(input: String): String {
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

    /** 复用 [com.kivo.cinema.smb.SmbAddress] 的解析规则，保证表单、存储、连接三处同一个标准。 */
    private fun normalizeSmb(input: String): String {
        val address = try {
            com.kivo.cinema.smb.SmbAddress.parse(input)
        } catch (e: IllegalArgumentException) {
            throw IllegalArgumentException(e.message ?: "请输入有效的 SMB 地址")
        }
        return address.toUri()
    }

    // 表单展开组（254 D13）：协议/端口从**唯一地址态**派生显示，编辑后写回同一串，不另立地址状态。
    private fun ensureScheme(address: String): String {
        val t = address.trim()
        return if (t.contains("://")) t else "https://$t"
    }

    /** 派生 http/https（缺省 https）；解析不出合法 scheme 时退 https，不抛。 */
    fun schemeOf(address: String): String =
        ensureScheme(address).substringBefore("://").lowercase(Locale.ROOT)
            .takeIf { it == "http" || it == "https" } ?: "https"

    /** 派生显式端口（无端口/默认端口留空，由 normalize 剥离）。 */
    fun portOf(address: String): String =
        runCatching { URI(ensureScheme(address)).port }.getOrNull()
            ?.takeIf { it in 1..65535 }?.toString() ?: ""

    /** 写回协议：只换 scheme，host/port/path 原样保留。 */
    fun withScheme(address: String, scheme: String): String {
        val t = address.trim()
        val rest = if (t.contains("://")) t.substringAfter("://") else t
        return "$scheme://$rest"
    }

    /** 写回端口：空串=去掉端口；非法端口忽略。host/path 原样保留。 */
    fun withPort(address: String, port: String): String {
        val t = ensureScheme(address)
        val scheme = t.substringBefore("://")
        val rest = t.substringAfter("://")
        val host = rest.substringBefore('/').substringBefore(':')
        val path = if (rest.contains('/')) "/" + rest.substringAfter('/') else ""
        val p = port.trim().toIntOrNull()?.takeIf { it in 1..65535 }
        return "$scheme://$host${p?.let { ":$it" }.orEmpty()}$path"
    }
}
