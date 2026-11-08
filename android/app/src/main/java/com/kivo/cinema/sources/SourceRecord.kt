package com.kivo.cinema.sources

import java.util.UUID

/** Deliberately not a data class: secrets must never enter generated toString/copy output. */
class SourceRecord(
    val id: String = UUID.randomUUID().toString(),
    val revision: Long = 1,
    val name: String,
    address: String,
    val username: String,
    val password: String,
    val hidden: Boolean = false,
    val defaultHome: Boolean = false,
    val aggregateEnabled: Boolean = true,
    val aggregateSearchEnabled: Boolean = true,
) {
    val address: String = SourceAddress.normalize(address)
    init {
        require(id.isNotBlank() && revision > 0) { "无效的源标识或版本" }
        require(name.isNotBlank()) { "请输入名称" }
        require(username.isNotBlank()) { "请输入账号" }
    }
    internal fun sameAccount(other: SourceRecord) =
        address == other.address && username == other.username
    internal fun withControls(hidden: Boolean = this.hidden, defaultHome: Boolean = this.defaultHome,
        aggregateEnabled: Boolean = this.aggregateEnabled,
        aggregateSearchEnabled: Boolean = this.aggregateSearchEnabled, revision: Long = this.revision) =
        SourceRecord(id, revision, name, address, username, password, hidden, defaultHome,
            aggregateEnabled, aggregateSearchEnabled)
    override fun toString() = "SourceRecord(redacted)"
}

class SourceFailure(val reason: String) : Exception(reason)
