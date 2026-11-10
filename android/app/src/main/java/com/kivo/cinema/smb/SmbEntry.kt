package com.kivo.cinema.smb

/**
 * SMB 目录条目（243 §3.2）。纯数据，无协议逻辑、无 UI 依赖。
 *
 * 原先借用旧 jcifs 后端的内部类，导致 libsmb2 后端反向依赖一个已废弃的类；
 * S1 起由本类型承载，后端与 UI 都只认它。
 */
data class SmbEntry(
    val name: String,
    val isDirectory: Boolean,
    val size: Long,
)
