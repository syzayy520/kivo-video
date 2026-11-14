package com.kivo.cinema.net

import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

/**
 * 273 v18：**建连** —— 打开一条到源站的 HTTP 连接（含重定向与状态码翻译）。
 *
 * ## 为什么从 [HttpOriginClient] 里分出来
 * "怎么把连接建起来"（协议、重定向、超时、状态码语义）与
 * "怎么把字节读回来"（范围、分块、收尾）是**两种变化原因**：
 * 换源站协议不该动读流逻辑，反之亦然。
 *
 * ## 池参数必须在第一次建连前设好
 * JDK 的连接池默认只有 5 条连接、闲置 5 秒就回收。我们同时有主播放、
 * 拖动预览、每次 seek 的新连接 —— 5 条不够（超出的请求会排队等空闲连接，
 * 表现为莫名其妙的卡顿），闲置 5 秒也短（环满时连接闲置常常超过 5 秒）。
 */
internal object OriginHttpConnection {

    /**
     * 打开一条连接（跟随重定向，最多 [MAX_REDIRECTS] 跳）。
     *
     * @param endInclusive null = 开放区间（读到文件尾）。
     * @throws IOException 重定向异常（缺 Location / 跳太多）。
     */
    fun open(url: String, token: String?, start: Long, endInclusive: Long?): HttpURLConnection {
        var target = url
        var hops = 0
        while (true) {
            val conn = URL(target).openConnection() as HttpURLConnection
            conn.instanceFollowRedirects = false
            conn.connectTimeout = CONNECT_TIMEOUT_MS
            conn.readTimeout = READ_TIMEOUT_MS
            // 必须是 identity：压缩过的字节流与文件的绝对偏移对不上。
            conn.setRequestProperty("Accept-Encoding", "identity")
            conn.setRequestProperty("User-Agent", "kivo-video/1.0")
            conn.setRequestProperty(
                "Range",
                if (endInclusive == null) "bytes=$start-" else "bytes=$start-$endInclusive",
            )
            token?.let { conn.setRequestProperty("X-Emby-Token", it) }

            val code = conn.responseCode
            if (code !in 300..399) return conn

            val location = conn.getHeaderField("Location")
            conn.disconnect()
            if (location.isNullOrBlank() || ++hops > MAX_REDIRECTS) {
                throw IOException("源站重定向异常（HTTP $code）")
            }
            target = URL(URL(target), location).toString()
        }
    }

    /** 源站的状态码 → 用户看得懂的话（不许静默降级，G9）。 */
    fun checkCode(code: Int) {
        when (code) {
            401, 403 -> throw IllegalStateException("服务器拒绝了访问（HTTP $code），请检查账号或权限")
            404 -> throw IllegalStateException("服务器上没有找到这个视频（HTTP 404）")
            206, 200 -> Unit
            else -> throw IllegalStateException("服务器不支持范围读取（HTTP $code），无法播放")
        }
    }

    private const val CONNECT_TIMEOUT_MS = 10_000
    private const val READ_TIMEOUT_MS = 30_000
    private const val MAX_REDIRECTS = 3

    /**
     * 连接池参数：必须在**第一次建连之前**设好
     * （JDK 的 KeepAliveCache / HttpClient 都是静态初始化的）。
     *
     * · `http.maxConnections` 默认 5 —— 我们同时有主播放、拖动预览、
     *   每次 seek 的新连接，5 条不够；超出的请求会**排队等空闲连接**。
     * · `http.keepAlive.timeout` 默认 5000 ms —— 环满时连接会闲置更久，
     *   池里的连接被回收后，下一次请求又要重新握手（真机 754–968 ms）。
     */
    init {
        System.setProperty("http.maxConnections", "16")
        System.setProperty("http.keepAlive.timeout", "60000")
    }
}
