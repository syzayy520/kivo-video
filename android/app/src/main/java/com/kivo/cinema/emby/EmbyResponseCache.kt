package com.kivo.cinema.emby

import java.io.File
import java.security.MessageDigest

/**
 * Emby **慢变元数据**响应磁盘缓存（D366，对标 Rex 的 Data Request Cache / Detail cache duration）。
 *
 * ## 为什么需要
 * 真机实测详情页每一跳都要重打源站（`detail-perf` 埋点）：
 * 电影 1.33s 里 Emby 详情占 908ms，剧集 2.38s 里季集占 1592ms。
 * Rex 的详情页"进去就已经加载好"，靠的正是详情/数据分域持久化缓存；
 * 我们此前只缓存了 TMDB JSON，Emby 这一侧零缓存 ⇒ 二次进入仍要重等。
 *
 * ## 缓存什么、不缓存什么
 * **缓存**（慢变数据，重复进入不变）：详情元数据、图片尺寸表、季列表、集列表。
 * **绝不缓存**（快变/有副作用）：`Items/Resume`、`Items/Counts`、`Users/.../Items` 列表、
 * `Playing|Played|Favorite` 等写操作 —— 缓存它们会让首页续播与已看状态定格。
 *
 * ## 一致性
 * 缓存里含 `UserData`（收藏/已看/续播位）。任何写操作都调 [invalidateItem] 把该条的
 * 详情与季集缓存删掉，下次进入回源；宁可多一次往返，也不给用户看过期状态。
 *
 * ## 边界（不静默降级）
 * 只写 2xx 正文；失败/空响应永不落盘。原子写（临时文件 + rename），避免半截 JSON。
 * 未初始化（无 cacheDir）时退化为不缓存、直接回源，绝不假装命中。
 */
object EmbyResponseCache {

    /** 详情元数据：6 小时。含 UserData，写操作会主动失效。 */
    const val TTL_DETAILS = 6L * 3600 * 1000
    /** 季/集列表：6 小时。含每集进度，写操作会主动失效。 */
    const val TTL_EPISODES = 6L * 3600 * 1000
    /** 图片尺寸表：30 天（图片换了 tag 才会变，与本表无关）。 */
    const val TTL_IMAGE_WIDTHS = 30L * 24 * 3600 * 1000
    /**
     * 列表/计数：10 分钟。
     *
     * 【为什么短】列表会因服务器新增影片而变；真机实测 Emby 浏览页**每个分类行一次请求**，
     * 每次 0.4–0.9 s，滚动/返回都会重打。10 分钟内复用足够覆盖"来回浏览"，又不至于
     * 新片入库后长时间看不到。
     */
    const val TTL_LIST = 10L * 60 * 1000

    private var dir: File? = null

    /** 幂等；由持有 Context 的入口调用（Activity onCreate 或详情取数前）。 */
    @Synchronized
    fun init(cacheDir: File) {
        if (dir == null) dir = File(cacheDir, "emby-json-v1").apply { mkdirs() }
    }

    /** 命中且未过期返回正文；缺失/过期/读失败返回 null，由调用方回源。 */
    fun get(url: String, ttlMs: Long): String? {
        val f = file(url) ?: return null
        if (!f.isFile || System.currentTimeMillis() - f.lastModified() > ttlMs) return null
        val text = runCatching { f.readText() }.getOrNull().takeUnless { it.isNullOrEmpty() }
        log("${if (text != null) "HIT" else "MISS"} ${short(url)}")
        return text
    }

    /** 只写成功正文；空串不落盘。原子替换。同时写一份 `.key`（存原 URL）供失效按 id 精确删除。 */
    fun put(url: String, body: String) {
        if (body.isEmpty()) return
        val target = file(url) ?: return
        log("STORE ${short(url)}")
        runCatching {
            // 目录可能已被系统清缓存/被删掉（cacheDir 随时可能被回收）⇒ 每次写前确保存在，
            // 否则静默写失败，缓存永远 MISS（真机踩过）。
            target.parentFile?.mkdirs()
            val tmp = File.createTempFile("embyjson-", ".tmp", target.parentFile)
            try {
                tmp.writeText(body)
                if (!tmp.renameTo(target)) {
                    tmp.delete()
                    target.writeText(body)
                }
            } finally {
                tmp.delete()
            }
            // .key 只存 URL（不含 token），让失效无需读正文。
            File(target.parentFile, target.name + KEY_SUFFIX).writeText(url)
        }
    }

    /**
     * 写操作后让该影片的缓存失效。
     *
     * 详情 URL 与季/集 URL 都含 `itemId`，故按 `.key` 里的原 URL 匹配删除。
     * 只读 `.key` 小文件，不读正文；多删无副作用（下次回源一次）。
     */
    fun invalidateItem(itemId: String) {
        val root = dir ?: return
        if (itemId.isBlank()) return
        var removed = 0
        runCatching {
            root.listFiles { f -> f.name.endsWith(KEY_SUFFIX) }?.forEach { keyFile ->
                val url = runCatching { keyFile.readText() }.getOrNull() ?: return@forEach
                if (!url.contains(itemId)) return@forEach
                File(keyFile.parentFile, keyFile.name.removeSuffix(KEY_SUFFIX)).delete()
                keyFile.delete()
                removed++
            }
        }
        if (removed > 0) log("INVALIDATE item=$itemId removed=$removed")
    }

    /** 日志只留路径片段（剥掉 userId 前后噪音），不含 token —— 本缓存键里本就没有 token。 */
    private fun short(url: String): String =
        url.substringAfter("/emby", url).substringAfterLast('/').take(48)
            .ifBlank { url.takeLast(48) }

    private fun log(msg: String) = com.kivo.cinema.net.NetDiag.log("[emby-cache] $msg")

    private fun file(url: String): File? = dir?.let { root ->
        val key = MessageDigest.getInstance("SHA-256").digest(url.toByteArray(Charsets.UTF_8))
            .joinToString("") { b -> "%02x".format(b) }
        File(root, key)
    }

    /** 伴生键文件后缀（只存原 URL，供按 id 失效）。 */
    private const val KEY_SUFFIX = ".key"
}
