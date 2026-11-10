package com.kivo.cinema.smb

/**
 * 系统 errno → 人话（250 D9）。纯映射，无 IO。
 *
 * 为什么与 [SmbError] 分家：两者说的**不是同一件事**。
 * - [SmbError] 翻译"服务端告诉我们什么"：账号错、共享没授权、文件不存在；
 * - 这里翻译"本机 socket 层出了什么事"：网线掉了、连接被重置、对端超时。
 *
 * 合成一个 `when` 会得到一处既懂 NT status 又懂 errno 的映射表，
 * 两个完全不同的上游（SMB 报文 / 内核返回值）被压进同一个修改点 —— 改任何一个都得动它。
 *
 * 未收录的 errno 保留数字：取证时能与源码逐字对上，比"读取失败"有用。
 */
internal object SmbErrno {

    // Linux / Android errno。libsmb2 的 sync 层在报文还没发出去时直接返回 -errno。
    private const val EPERM = 1
    private const val ENOENT = 2
    private const val EIO = 5
    private const val EACCES = 13
    private const val EPIPE = 32
    private const val ENETUNREACH = 101
    private const val ECONNRESET = 104
    private const val ETIMEDOUT = 110
    private const val EHOSTUNREACH = 113

    fun text(errno: Int): String = when (errno) {
        ENOENT -> "文件不存在，可能已被移动或删除"
        EACCES, EPERM -> "没有读取这个文件的权限"
        EIO -> "读取中断，请检查网络后重试"
        ECONNRESET, EPIPE -> "连接被服务器断开"
        ETIMEDOUT -> "连接超时，网络可能不稳定"
        ENETUNREACH, EHOSTUNREACH -> "网络不可达，请检查 Wi-Fi 或网线"
        else -> "读取失败（系统错误 $errno）"
    }
}
