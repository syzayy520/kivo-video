package com.kivo.cinema.smb

import java.io.IOException

/**
 * 一条可重连的读通道（250 D4）：一次读 + 断线重连 + 退避。
 *
 * 从 `SmbByteSource` 剥出来的理由：这是**唯一**会碰网络的单元，也是唯一会因为
 * "Wi-Fi 抖动 / 硬盘休眠 / 服务器重启"而改变行为的单元。窗口、预读、统计都不该
 * 知道"重连"这件事 —— 它们只该知道"这次读拿到了 / 没拿到"。
 *
 * 三条从 243 的真实故障里得来的纪律（不是设计偏好）：
 * 1. **死句柄不许再被读**。重连失败时旧 `SmbReader` 已 `close()`，但字段还指着它；
 *    再读只会拿到"读取器尚未打开"，把真实的"网络还没回来"整句盖掉。
 * 2. **重连失败不进入终态**。记下真实原因、按退避推后下一次；"什么时候放弃"由
 *    fd 层的耐心窗口决定，不在这里。
 * 3. **重连成功后旧数据一律作废**（通过 [onReconnected] 通知窗口清空）。
 */
internal class SmbPreadChannel(
    private val address: SmbAddress,
    private val credential: SmbCredential,
    private val filePath: String,
    private val gate: SmbRetryGate,
    initialReader: SmbReader,
    private val nowMs: () -> Long = { System.nanoTime() / 1_000_000 },
    private val onReconnected: () -> Unit = {},
    /** 每拿到 n 字节回报一次（诊断用的带宽计数，不参与任何决策）。 */
    private val onBytes: (Int) -> Unit = {},
) {
    private var reader: SmbReader = initialReader
    private var dead = false

    /**
     * 一次读。返回字节数；`<0` 表示 EOF。
     *
     * 失败时抛 [IOException]：要么"现在不该试"（闸门说不），要么"重连也失败"。
     * 两种情况都带一句用户能看懂的话，绝不静默。
     */
    fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        val now = nowMs()
        if (!gate.canAttempt(now)) throw IOException(gate.denyReason())

        if (!dead) {
            try {
                val read = reader.pread(offset, buf, off, len)
                if (read > 0) {
                    gate.noteSuccess()
                    onBytes(read)
                }
                return read
            } catch (_: Exception) {
                // 落到重连路径。真实原因由重连结果决定：成功就没事，
                // 失败会带上"连不上"的原因，比这里的 socket 报错更有用。
            }
        }

        runCatching { reader.close() }
        dead = true
        val fresh = SmbReader(address, credential, filePath)
        try {
            fresh.open()
        } catch (failed: Exception) {
            val reason = "SMB 连接已断开，重连失败：${failed.message?.take(80) ?: "未知原因"}"
            gate.noteFailure(now, reason)
            throw IOException(reason, failed)
        }
        reader = fresh
        dead = false
        gate.noteSuccess()
        onReconnected()
        return fresh.pread(offset, buf, off, len).also { if (it > 0) onBytes(it) }
    }

    /**
     * 把 [len] 字节尽量读满，只有真 EOF 才提前结束；一个字节都没读到返回 -1。
     *
     * 把"读满"的语义收在这里，而不是让调用方各自写循环：未到尾部却读到 0 必须当
     * 传输异常抛出去，**绝不能返回给上层当 EOF** —— 上一版就是因为某处把 0 当 EOF，
     * 播放器认为"片源结束"，画面静默停住。这条规则只该有一个实现。
     */
    fun readFully(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        var done = 0
        while (done < len) {
            val read = readAt(offset + done, buf, off + done, len - done)
            if (read < 0) break
            if (read == 0) throw IOException("SMB 读取中断（${offset + done} 字节处返回空数据）")
            done += read
        }
        return if (done == 0) -1 else done
    }

    fun close() {
        dead = true
        runCatching { reader.close() }
    }
}
