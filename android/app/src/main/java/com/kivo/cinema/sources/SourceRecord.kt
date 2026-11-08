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
) {
    val address: String = SourceAddress.normalize(address)
    init {
        require(id.isNotBlank() && revision > 0) { "无效的源标识或版本" }
        require(name.isNotBlank()) { "请输入名称" }
        require(username.isNotBlank()) { "请输入账号" }
    }
    internal fun sameAccount(other: SourceRecord) =
        address == other.address && username == other.username
    override fun toString() = "SourceRecord(redacted)"
}

class SourceFailure(val reason: String) : Exception(reason)
