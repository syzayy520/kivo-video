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
            // 【274 · 不复用连接】见 [reuseConnection]：源站的**单条连接吞吐会随时间衰减**，
            // 复用一条连接继续取块 = 越取越慢（实测 226 → 42 KB/s），
            // 换一条新连接立刻恢复（200–680 KB/s）。这一行就是关掉 JDK 池化的开关。
            if (!reuseConnection) conn.setRequestProperty("Connection", "close")

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
     * 是否**复用**源站连接（keep-alive）。
     *
     * 【默认否】274 真机 + PC 对照实测（设计稿 `273-emby-stutter-fix.md` §二十八）：
     * 同一条 keep-alive 连接连续取 8 个 4 MiB 块，正文速率
     * **226 → 150 → 100 → 50 → 48 → 47 → 42 → 67 KB/s**（表头始终 300–1300 ms，
     * 说明连接是活的，衰减发生在**正文吞吐**上）；改成每块新建连接后
     * **203 / 630 / 372 / 678 KB/s**。差 5–10 倍。
     *
     * 【代价】每块多一次 TLS 握手（300–1800 ms）。一块 4 MiB 在 500 KB/s 下要 8 秒
     * ⇒ 握手占 4–20%，而**衰到 40 KB/s 是 10 倍的损失**，这笔账怎么算都该换连接。
     *
     * 【为什么要开关】"复用更快"曾被真机实测支持过（v5：复用 300 ms/10 MB/s，
     * 新建 754–968 ms/2 MB/s）⇒ 这个结论**会被源站策略推翻**。
     * 留 `debug.kivo.originKeepAlive=1` 一个回退臂，才能在源站改变行为时
     * 用一条命令回到旧行为，而不是靠改代码重打包。
     */
    private val reuseConnection: Boolean by lazy {
        readSystemProperty(KEEPALIVE_PROPERTY)?.trim() == "1"
    }

    /** 系统属性名：置 `1` ⇒ 复用连接（旧行为 / 回退臂）。 */
    private const val KEEPALIVE_PROPERTY = "debug.kivo.originKeepAlive"

    /**
     * 读 Android 系统属性（`adb shell setprop <name> <value>` 即可生效）。
     *
     * 【为什么走反射】`android.os.SystemProperties` 是隐藏 API，编译期不可见。
     * 非 Android 环境（JVM 单测）读不到 ⇒ 返回 null 走默认，不抛。
     */
    private fun readSystemProperty(name: String): String? = try {
        val clazz = Class.forName("android.os.SystemProperties")
        clazz.getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }

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
