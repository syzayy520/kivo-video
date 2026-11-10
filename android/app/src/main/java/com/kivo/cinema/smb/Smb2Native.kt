package com.kivo.cinema.smb

/**
 * libsmb2 的 JNI 声明（243 起为唯一 native 入口）。
 *
 * 这一层只声明"能做什么"，不持有状态、不做错误翻译——连接与生命周期归
 * [SmbSession] / [SmbReader]，错误分类归 [SmbError]。
 *
 * 错误纪律（G9）：native 返回负值/空时，调用方必须用 [nativeNtStatus] +
 * [nativeGetError] 组装用户可见 reason，不允许吞掉。
 */
object Smb2Native {

    init {
        System.loadLibrary("kivo_player")
    }

    external fun nativeCreate(): Long
    external fun nativeDestroy(ctx: Long)

    /**
     * 把读阶段超时改成 [seconds]（250 D2）。必须在 connect / open 成功之后调用 ——
     * 那两个阶段需要更长的耐心（慢网络上的认证与树连接），只有 READ 该用短超时。
     */
    external fun nativeSetReadTimeout(ctx: Long, seconds: Int): Int

    /** 连到 [share]；返回 0 成功。share 用 "IPC$" 时是枚举共享的前置连接。 */
    external fun nativeConnect(ctx: Long, host: String, port: Int, share: String,
                               user: String, pass: String, domain: String): Int

    /** 列目录，返回 JSON 数组 [{"name","dir","size"}]；失败返回 null。 */
    external fun nativeList(ctx: Long, path: String): String?

    /** 枚举共享（必须已连 IPC$），返回 JSON [{"name","remark","type"}]；失败返回 null。 */
    external fun nativeListShares(ctx: Long): String?

    external fun nativeOpen(ctx: Long, path: String): Long
    external fun nativeSize(ctx: Long, fh: Long): Long

    /** 读进新数组（兼容路径，S2 起播放走 [nativeReadInto]）。 */
    external fun nativeReadAt(ctx: Long, fh: Long, offset: Long, len: Int): ByteArray?

    /** 直接读进调用方的 ByteArray；返回字节数，0 = EOF，<0 = 失败。 */
    external fun nativeReadInto(ctx: Long, fh: Long, offset: Long, buf: ByteArray,
                                off: Int, len: Int): Int

    external fun nativeCloseFh(ctx: Long, fh: Long): Int

    external fun nativeGetError(ctx: Long): String

    /** 最近一次失败的 NT status（0xC00000xx）；配合 [SmbError.translate]。 */
    external fun nativeNtStatus(ctx: Long): Int
}
