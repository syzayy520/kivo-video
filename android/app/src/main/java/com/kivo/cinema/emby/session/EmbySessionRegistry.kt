package com.kivo.cinema.emby.session

import com.kivo.cinema.emby.EmbyCatalog
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyHeroItem
import com.kivo.cinema.emby.library.EmbyLibraryModel

/**
 * 338 · 一个**已连接** Emby 服的可恢复会话（纯数据，不发起网络、不持有生命周期）。
 *
 * 为什么按服缓存：切服原本重启 Activity，新 ViewModel 从零 login+items+hero+resume，
 * 界面先闪"正在读取资源库 / 正在连接资源库"，用户感知就是"又回资源库走一遍"。
 * 缓存住已连接服的完整状态后，来回切可原样恢复（含列表滚动位置、查询条件、评分补全结果）。
 *
 * 安全：只驻留内存、不写盘。不含密码——恢复时凭据从 SourceStore 现读；
 * token 随 [auth] 驻留，生命周期与 ViewModel 相同（onCleared/back 即清）。
 */
internal class EmbyServerSession(
    val client: EmbyClient,
    val auth: EmbyClient.Auth,
    val list: EmbyLibraryModel.Snapshot,
    val views: List<EmbyCatalog.View>,
    val heroes: List<EmbyHeroItem>,
    val resumed: List<EmbyHeroItem>,
    val savedName: String,
)

/** 338 · 按 sourceId 缓存已连接会话，LRU 上限 4（用户实际服数远小于此）。 */
internal class EmbySessionRegistry(private val max: Int = 4) {
    private val map = object : LinkedHashMap<String, EmbyServerSession>(max, 1f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<String, EmbyServerSession>?) = size > max
    }

    @Synchronized fun save(id: String, session: EmbyServerSession) { map[id] = session }

    @Synchronized fun find(id: String): EmbyServerSession? = map[id]

    @Synchronized fun clear() { map.clear() }
}
