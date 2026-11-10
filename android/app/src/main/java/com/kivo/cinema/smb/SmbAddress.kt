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

    /**
     * `host:port` 的唯一正确形态（250 D10）。交给 JNI 的东西只有这一种，不再在 native 侧拼串。
     *
     * 两条硬要求：
     * - **IPv6 必须带方括号**。libsmb2 的 `socket.c` 只有在首字符是 `[` 时才按 IPv6 解析，
     *   否则在第一个 `:` 处切分 —— `::1:445` 会被切成 host=`::1`、port=`1:445`，
     *   然后 `getaddrinfo` 失败。上一版直接拼 `h + ":" + port`，所以 IPv6 从来没通过。
     * - **链路本地的 `%wlan0` 必须保留**。zone id 是"从哪个网卡出去"，去掉它
     *   `fe80::` 开头的地址不可路由。
     */
    val authority: String
        get() = buildString {
            if (host.indexOf(':') >= 0) append('[').append(host).append(']') else append(host)
            append(':').append(port)
        }

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
