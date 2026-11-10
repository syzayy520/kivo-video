package com.kivo.cinema.smb

/**
 * SMB 共享条目（243 D1）。来自 `srvsvc NetrShareEnum`（libsmb2 `smb2_share_enum_sync`）。
 *
 * `type` 是 Windows 的 STYPE 位域：
 * - 低 2 位：0 磁盘、1 打印队列、2 设备、3 IPC
 * - 0x40000000 临时共享、0x80000000 隐藏共享（名字以 `$` 结尾）
 *
 * 用户只会看到磁盘共享；`IPC$` 之类必须过滤掉，否则列表里全是噪音。
 */
data class SmbShare(
    val name: String,
    val remark: String,
    val type: Int,
) {
    val isDisk: Boolean get() = (type and TYPE_MASK) == TYPE_DISK

    val isHidden: Boolean get() = (type.toLong() and HIDDEN_FLAG) != 0L

    /** 可展示给用户的共享（磁盘且非 IPC/打印/设备）。 */
    val isBrowsable: Boolean get() = isDisk && !name.endsWith("$", ignoreCase = true)

    companion object {
        private const val TYPE_MASK = 0x3
        private const val TYPE_DISK = 0
        private const val HIDDEN_FLAG = 0x80000000L
    }
}
