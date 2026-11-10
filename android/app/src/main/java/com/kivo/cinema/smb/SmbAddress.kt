package com.kivo.cinema.smb

/**
 * SMB 地址（243 D2）。纯解析：不联网、不依赖 Android、不碰 UI。
 *
 * 接受的形式（大小写不敏感，`\` 与 `/` 等价）：
 * - `smb://host`、`smb://host:1445`、`smb://host/share`、`smb://host/share/电影/2024`
 * - `\\host\share\path`、`host/share`、`host:1445/share`
 *
 * 语义：`share` 为空表示"还没选共享"，由 UI 触发枚举后回填；
 * `path` 是共享内路径，空表示共享根。名称大小写与空格原样保留（SMB 文件名可含空格）。
 */
data class SmbAddress(
    val host: String,
    val port: Int = DEFAULT_PORT,
    val share: String = "",
    val path: String = "",
) {
    /** 还没有指定共享——需要先枚举再让用户选。 */
    val needsShare: Boolean get() = share.isEmpty()

    /** 完整共享内路径（libsmb2 要的形态，不带前导斜杠）。 */
    fun filePath(extra: String = ""): String =
        listOf(share, path, extra.trim('/'))
            .filter { it.isNotEmpty() }
            .joinToString("/")

    /** 规范化的展示/存储形态（默认端口不写出来）。 */
    fun toUri(): String = buildString {
        append("smb://").append(if (host.contains(':')) "[$host]" else host)
        if (port != DEFAULT_PORT) append(':').append(port)
        if (share.isNotEmpty()) append('/').append(share)
        if (path.isNotEmpty()) append('/').append(path.trim('/'))
    }

    companion object {
        const val DEFAULT_PORT = 445

        fun parse(input: String): SmbAddress {
            val raw = input.trim()
            require(raw.isNotEmpty()) { "请输入 SMB 地址" }
            var s = if (raw.startsWith("smb://", ignoreCase = true)) raw.substring(6) else raw
            s = s.replace('\\', '/')
            // UNC 的 \\host\share 转成 //host/share，这一对斜杠是"根"不是分隔符
            if (s.startsWith("//")) s = s.substring(2)
            require(!s.startsWith("/")) { "地址缺少服务器" }
            require(s.isNotEmpty()) { "请输入 SMB 地址" }

            val slash = s.indexOf('/')
            val authority = if (slash >= 0) s.substring(0, slash) else s
            val rest = if (slash >= 0) s.substring(slash + 1) else ""
            require(authority.isNotBlank()) { "地址缺少服务器" }

            val host: String
            val port: Int
            if (authority.startsWith("[")) {
                val close = authority.indexOf(']')
                require(close > 1) { "IPv6 地址格式不正确" }
                host = authority.substring(1, close)
                val after = authority.substring(close + 1)
                port = when {
                    after.isEmpty() -> DEFAULT_PORT
                    after.startsWith(":") -> portOf(after.substring(1))
                    else -> throw IllegalArgumentException("地址格式不正确")
                }
            } else {
                val colon = authority.lastIndexOf(':')
                if (colon >= 0) {
                    host = authority.substring(0, colon)
                    port = portOf(authority.substring(colon + 1))
                } else {
                    host = authority
                    port = DEFAULT_PORT
                }
            }
            require(host.isNotBlank()) { "地址缺少服务器" }

            val segments = rest.split('/').filter { it.isNotEmpty() }
            return SmbAddress(
                host = host,
                port = port,
                share = segments.firstOrNull().orEmpty(),
                path = segments.drop(1).joinToString("/"),
            )
        }

        private fun portOf(text: String): Int {
            val value = text.toIntOrNull() ?: throw IllegalArgumentException("端口应为 1–65535")
            require(value in 1..65535) { "端口应为 1–65535" }
            return value
        }
    }
}
