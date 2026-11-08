package com.kivo.cinema.sources

import java.net.URI
import java.net.URLDecoder

/** Pure, explicit paste preview. No clipboard access, network, or implicit connection. */
object SourceShareParser {
    class Preview(
        val addresses: List<String>,
        val name: String?,
        val username: String?,
        val password: String?,
        val issues: List<String>,
    ) {
        val canApply: Boolean get() = addresses.size == 1 && issues.isEmpty()
        override fun toString() = "SourceSharePreview(redacted)"
    }

    fun parse(text: String): Preview {
        val fields = mutableMapOf<String, String>()
        val candidates = linkedSetOf<String>()
        val explicitPorts = mutableListOf<Int>()
        val issues = linkedSetOf<String>()
        fun field(key: String, value: String) {
            if (fields.containsKey(key) && fields[key] != value) issues += "同一字段存在冲突，请手动核对"
            else fields[key] = value
        }
        fun address(raw: String) {
            try {
                val input = raw.trim()
                val uri = URI(if (input.contains("://")) input else "https://$input")
                if (uri.port != -1) explicitPorts += uri.port
                var clean = input
                uri.rawUserInfo?.let { info ->
                    val split = info.indexOf(':')
                    field("username", decode(if (split < 0) info else info.substring(0, split)))
                    if (split >= 0) field("password", decode(info.substring(split + 1)))
                    clean = "${uri.scheme}://${uri.rawAuthority.substringAfterLast('@')}${uri.rawPath.orEmpty()}" +
                        (uri.rawQuery?.let { "?$it" } ?: "") + (uri.rawFragment?.let { "#$it" } ?: "")
                }
                candidates += SourceAddress.normalize(clean)
            } catch (_: Exception) { issues += "有地址无法识别，请检查完整地址" }
        }
        text.lineSequence().forEach { line ->
            val match = label.matchEntire(line)
            if (match != null) {
                val key = when (match.groupValues[1].lowercase()) {
                    "名称", "名字", "name" -> "name"
                    "地址", "服务器", "服务器地址", "url", "server", "address" -> "address"
                    "账号", "帐号", "用户", "用户名", "username", "user" -> "username"
                    "密码", "password", "pass" -> "password"
                    else -> "port"
                }
                val raw = match.groupValues[2]
                if (key == "address") address(raw)
                else field(key, if (key == "password") raw else raw.trim())
            } else {
                val trimmed = line.trim()
                val urls = url.findAll(trimmed).map { it.value }.toList()
                if (urls.isNotEmpty()) urls.forEach(::address)
                else if (bare.matches(trimmed)) address(trimmed)
            }
        }
        fields["port"]?.let { raw ->
            val port = raw.toIntOrNull()
            if (port == null || port !in 1..65535) issues += "端口应为 1–65535"
            else {
                val updated = candidates.map { candidate ->
                    val uri = URI(candidate)
                    // Explicit default ports were normalized away; inspect raw labeled/URL text too.
                    if ((uri.port != -1 && uri.port != port) || explicitPorts.any { it != port }) {
                        issues += "地址与独立端口存在冲突，请手动核对"
                        candidate
                    } else SourceAddress.normalize("${uri.scheme}://${uri.host}:$port${uri.rawPath.orEmpty()}")
                }
                candidates.clear()
                candidates.addAll(updated)
            }
        }
        if (candidates.isEmpty()) issues += "未找到服务器地址"
        if (candidates.size > 1) issues += "检测到多个地址，请选择后再连接"
        return Preview(candidates.toList(), fields["name"], fields["username"], fields["password"], issues.toList())
    }

    private fun decode(raw: String): String = URLDecoder.decode(raw.replace("+", "%2B"), "UTF-8")
    private val label = Regex("^\\s*(名称|名字|name|地址|服务器地址|服务器|url|server|address|账号|帐号|用户|用户名|username|user|密码|password|pass|端口|port)\\s*[:：](.*)$", RegexOption.IGNORE_CASE)
    private val url = Regex("https?://[^\\s<>\"，；]+", RegexOption.IGNORE_CASE)
    private val bare = Regex("(?:[A-Za-z0-9-]+\\.)+[A-Za-z0-9-]+(?::[0-9]+)?(?:/[^\\s]*)?")
}
