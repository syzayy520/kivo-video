package com.kivo.cinema.sources

import java.util.UUID

/**
 * 一个已保存的内容源（243 D5）。`protocol` 决定地址形态与验证方式：
 * - `emby`：http(s) 服务地址，账号必填
 * - `smb`：`smb://host[:port][/share][/path]`，**允许匿名**（用户名空）
 *
 * Deliberately not a data class: secrets must never enter generated toString/copy output.
 */
class SourceRecord(
    val id: String = UUID.randomUUID().toString(),
    val revision: Long = 1,
    val name: String,
    address: String,
    val username: String,
    val password: String,
    val protocol: String = PROTOCOL_EMBY,
    val hidden: Boolean = false,
    val defaultHome: Boolean = false,
    val aggregateEnabled: Boolean = true,
    val aggregateSearchEnabled: Boolean = true,
    val note: String = "",
) {
    val address: String = SourceAddress.normalize(address, protocol)

    init {
        require(id.isNotBlank() && revision > 0) { "无效的源标识或版本" }
        require(name.isNotBlank()) { "请输入名称" }
        require(protocol in PROTOCOLS) { "不支持的源类型" }
        if (protocol == PROTOCOL_EMBY) require(username.isNotBlank()) { "请输入账号" }
    }

    val isSmb: Boolean get() = protocol == PROTOCOL_SMB

    internal fun sameAccount(other: SourceRecord) =
        protocol == other.protocol && address == other.address && username == other.username

    internal fun withControls(hidden: Boolean = this.hidden, defaultHome: Boolean = this.defaultHome,
        aggregateEnabled: Boolean = this.aggregateEnabled,
        aggregateSearchEnabled: Boolean = this.aggregateSearchEnabled, revision: Long = this.revision) =
        SourceRecord(id, revision, name, address, username, password, protocol, hidden, defaultHome,
            aggregateEnabled, aggregateSearchEnabled, note)

    override fun toString() = "SourceRecord($protocol, redacted)"

    companion object {
        const val PROTOCOL_EMBY = "emby"
        const val PROTOCOL_SMB = "smb"
        val PROTOCOLS = setOf(PROTOCOL_EMBY, PROTOCOL_SMB)
    }
}

class SourceFailure(val reason: String) : Exception(reason)
