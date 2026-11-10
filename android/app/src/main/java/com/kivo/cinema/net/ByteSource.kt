package com.kivo.cinema.net

/**
 * 随机读源契约（243 D3）——SMB / WebDAV / 本地文件唯一的共同接口。
 *
 * 语义是本文件定义的，实现方必须遵守，调用方只按这里写的方式判断：
 * - [size]：总字节数。读不到就抛异常，**不返回 -1 当哨兵**。
 * - [readAt]：返回实际读到的字节数；已到文件尾返回 `-1`。
 *   参数越界或 `len == 0` 时返回 0，不算错误。
 * - 任何 I/O 失败抛异常，由上层转成用户可见 reason（G9：不静默降级）。
 *
 * 线程模型：实现不保证线程安全，串行化由调用方负责（fd 回调层已持锁）。
 */
interface ByteSource : AutoCloseable {
    fun size(): Long

    fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int
}
