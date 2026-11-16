package com.kivo.cinema.tmdb

import java.io.File
import java.security.MessageDigest

/**
 * TMDB JSON 响应磁盘缓存（336；对标 Rex 的 Data Request Cache / Detail cache duration）。
 *
 * 单一职责：只做「响应正文 ↔ 磁盘文件」的存取，不解析 JSON、不发网络、不决定业务 TTL。
 * 缓存键 = 完整请求 URL（含 api_key）的 SHA-256；TTL 由调用方按数据类型传入：
 * 详情/搜索是慢变数据用长 TTL，首页列表更新快用短 TTL。
 *
 * 只写 2xx 正文；失败/空响应永不落盘（不静默降级：失败仍回源，错误由调用方原样上报）。
 * 原子写（临时文件 + rename），避免半截 JSON 被下次读到；解析失败可 [invalidate] 后回源。
 */
object TmdbResponseCache {
    /** 详情/搜索等慢变元数据：7 天。 */
    const val TTL_METADATA = 7L * 24 * 3600 * 1000
    /** 首页列表（热映/趋势/播出）：12 小时。 */
    const val TTL_LIST = 12L * 3600 * 1000

    private var dir: File? = null

    /** 由 TmdbClient 构造时注入（它总持有 Context），任何入口都覆盖。幂等。 */
    @Synchronized
    fun init(cacheDir: File) {
        if (dir == null) dir = File(cacheDir, "tmdb-json-v1").apply { mkdirs() }
    }

    /** 命中且未过期返回正文；缺失/过期/读取失败返回 null，由调用方回源。 */
    fun get(url: String, ttlMs: Long): String? {
        val f = file(url) ?: return null
        if (!f.isFile || System.currentTimeMillis() - f.lastModified() > ttlMs) return null
        val text = runCatching { f.readText() }.getOrNull()
        // 可观测性：HIT 才能证明「二次进详情页秒开」真的走缓存，不靠猜。
        com.kivo.cinema.net.NetDiag.log("[tmdb-cache] ${if (text != null) "HIT" else "FAIL"} ${short(url)}")
        return text
    }

    /** 只写成功正文；空串不落盘。原子替换。 */
    fun put(url: String, body: String) {
        if (body.isEmpty()) return
        val target = file(url) ?: return
        com.kivo.cinema.net.NetDiag.log("[tmdb-cache] STORE ${short(url)}")
        runCatching {
            val tmp = File.createTempFile("tmdbjson-", ".tmp", target.parentFile)
            try {
                tmp.writeText(body)
                if (!tmp.renameTo(target)) {
                    tmp.delete()
                    target.writeText(body)
                }
            } finally {
                tmp.delete()
            }
        }
    }

    /** 日志只留方法+id 片段，剥掉 api_key（NetDiag 不记任何凭据）。 */
    private fun short(url: String): String = url.substringAfter("/3/")
        .substringBefore("&api_key=").substringBefore("?api_key=").take(60)

    /** 解析失败/内容损坏时删除该项，让调用方本轮回源重建。 */
    fun invalidate(url: String) { file(url)?.let { runCatching { if (it.isFile) it.delete() } } }

    private fun file(url: String): File? = dir?.let { root ->
        val key = MessageDigest.getInstance("SHA-256").digest(url.toByteArray(Charsets.UTF_8))
            .joinToString("") { b -> "%02x".format(b) }
        File(root, key)
    }
}
