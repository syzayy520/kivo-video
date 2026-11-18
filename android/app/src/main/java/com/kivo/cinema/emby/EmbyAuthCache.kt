package com.kivo.cinema.emby

import com.kivo.cinema.sources.SourceRecord
import java.util.concurrent.ConcurrentHashMap

/**
 * Emby **已登录会话**的内存缓存（D367，由原 `EmbyResumeSession` 升级为通用）。
 *
 * ## 为什么值得
 * 真机实测 `[emby-connect] login 1221ms` —— 每次进 Emby 浏览页、每次进聚合搜索、
 * 首页拉续播，都要为每个来源各付一次 `AuthenticateByName` 往返。登录结果在 token 有效期内
 * 完全可复用，重复登录纯属浪费。
 *
 * ## 键怎么定
 * **按凭据**（地址 + 账号 + 密码的哈希）而不是按 sourceId：同一个服务器可能被用户删了又加
 * （id 变了但凭据没变），按凭据命中才不会白付一次登录。另外维护一份 sourceId → 凭据键的
 * 小索引，供只拿得到 sourceId 的场景（海报取图）使用。
 *
 * ## 边界（诚实）
 * - **只在内存**：不落盘、不进 `HomeEntry`；进程结束即失效，凭据不写任何文件。
 * - **TTL 15 分钟**：宁可偶发多一次登录，也不长期持有过期 token。
 * - 服务器侧**主动吊销 token**（如改密后踢下线）时，本缓存可能继续用旧 token 直到 TTL 到期；
 *   调用方遇到 401 应自行 [invalidate]（按 sourceId），不要假定本缓存永远有效。
 *
 * 单一职责：只做"凭据 → 已登录会话"的缓存与取图字节转发；不取数、不渲染、不落盘。
 */
object EmbyAuthCache {

    /** 会话有效期：15 分钟。 */
    private const val TTL_MS = 15L * 60 * 1000

    private class Entry(val client: EmbyClient, val auth: EmbyClient.Auth, val at: Long)

    /** 键 = 凭据哈希。 */
    private val byCred = ConcurrentHashMap<String, Entry>()

    /** sourceId → 凭据键（只用于"手里只有 sourceId"的调用方）。 */
    private val idToCred = ConcurrentHashMap<String, String>()

    private fun credKey(address: String, user: String, secret: String): String =
        "$address\u0000$user\u0000$secret".hashCode().toString()

    private fun lookup(key: String): Pair<EmbyClient, EmbyClient.Auth>? {
        val e = byCred[key] ?: return null
        if (System.currentTimeMillis() - e.at > TTL_MS) { byCred.remove(key); return null }
        return e.client to e.auth
    }

    /** 按源记录取会话（凭据变了会自然落到新键，旧条目随 TTL 淘汰）。 */
    fun get(record: SourceRecord): Pair<EmbyClient, EmbyClient.Auth>? =
        lookup(credKey(record.address, record.username, record.password))

    fun remember(record: SourceRecord, client: EmbyClient, auth: EmbyClient.Auth) {
        val key = credKey(record.address, record.username, record.password)
        byCred[key] = Entry(client, auth, System.currentTimeMillis())
        idToCred[record.id] = key
    }

    /** 只有散装凭据的场景（如 `EmbySessionModel.connectCore`，它拿不到 SourceRecord）。 */
    fun get(address: String, user: String, secret: String): Pair<EmbyClient, EmbyClient.Auth>? =
        lookup(credKey(address, user, secret))

    fun remember(address: String, user: String, secret: String,
                 client: EmbyClient, auth: EmbyClient.Auth) {
        byCred[credKey(address, user, secret)] = Entry(client, auth, System.currentTimeMillis())
    }

    /**
     * 取会话或登录（唯一入口）。
     *
     * 把"先查缓存、未命中才 login、登录后回填"收在这一处：调用方（浏览页/聚合搜索/首页续播）
     * 不必各自写一遍，也不会漏回填。
     */
    fun connect(address: String, user: String, secret: String): Pair<EmbyClient, EmbyClient.Auth> {
        get(address, user, secret)?.let { return it }
        val client = EmbyClient(address)
        val auth = client.login(user, secret)
        remember(address, user, secret, client, auth)
        return client to auth
    }

    /** 已知来源 id 的会话（海报取图等只需要 id 的场景）。 */
    fun session(sourceId: String): Pair<EmbyClient, EmbyClient.Auth>? =
        idToCred[sourceId]?.let { lookup(it) }

    /** 凭据失效/401 时调用：下次使用会重新登录。 */
    fun invalidate(sourceId: String) {
        idToCred.remove(sourceId)?.let { byCred.remove(it) }
    }

    /** 拉海报字节；会话缺失或任何失败返回 null（组件显示中性占位，不崩不白屏）。 */
    fun fetchBytes(sourceId: String?, url: String): ByteArray? {
        val s = session(sourceId ?: return null) ?: return null
        return runCatching { s.first.fetchBytes(url, s.second.token) }.getOrNull()
    }
}
