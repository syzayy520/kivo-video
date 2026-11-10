package com.kivo.cinema.smb

/**
 * SMB 播放数据连接（243 D4）：一条独立的 libsmb2 context + 一个打开的文件句柄。
 *
 * 与 [SmbSession]（控制面）分开的理由：浏览目录与播放取流如果共用一个 context，
 * 翻目录会把正在播的读请求堵住。代价是多一次认证往返，换来互不干扰。
 *
 * 线程模型：由 fd 回调线程串行调用，本类不做同步（[SmbByteSource] 已保证）。
 */
class SmbReader(
    private val address: SmbAddress,
    private val credential: SmbCredential,
    private val filePath: String,
) : AutoCloseable {

    private var ctx: Long = 0
    private var fh: Long = 0

    /** 打开成功后的文件大小（字节）。 */
    var size: Long = 0
        private set

    fun open() {
        check(ctx == 0L) { "SMB 读取器已打开" }
        val created = Smb2Native.nativeCreate()
        if (created == 0L) {
            throw SmbError.stageFailure(SmbStage.CONNECT, "无法初始化 SMB 会话")
        }
        ctx = created
        val rc = Smb2Native.nativeConnect(
            created, address.authority, address.share,
            credential.username, credential.password, credential.domain,
        )
        if (rc != 0) {
            val failure = translate(SmbStage.CONNECT)
            close()
            throw failure
        }
        val handle = Smb2Native.nativeOpen(created, filePath.trim('/'))
        if (handle == 0L) {
            val failure = translate(SmbStage.SHARE)
            close()
            throw failure
        }
        fh = handle
        val length = Smb2Native.nativeSize(created, handle)
        if (length < 0) {
            val failure = translate(SmbStage.READ)
            close()
            throw failure
        }
        size = length
        // 打开完成之后才把超时收窄到读窗口：connect / open 用 15 s 的耐心（慢网络上的
        // 认证与树连接真要几秒），READ 用 3 s —— 真机单次 1 MiB 最坏 300 ms 量级。
        // 这个值直接决定断网后"多久才知道读没回来"，是 P0-1 的上限。
        // 设不上就不能静默：那等于悄悄退回 15 秒，用户看到的是"卡住十几秒没反应"。
        val timeoutRc = Smb2Native.nativeSetReadTimeout(created, READ_TIMEOUT_SECONDS)
        require(timeoutRc == 0) { "无法设置 SMB 读超时（rc=$timeoutRc）" }
    }

    /**
     * 从 [offset] 读最多 [len] 字节到 [buf] 的 [off] 起。
     * 返回实际字节数；0 表示已到文件尾（由调用方转成 ByteSource 的 -1）。
     */
    fun pread(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        check(ctx != 0L && fh != 0L) { "SMB 读取器尚未打开" }
        val read = Smb2Native.nativeReadInto(ctx, fh, offset, buf, off, len)
        if (read < 0) {
            // 250 D9：用返回码本身归因。拿 smb2_get_nterror 兜底会把"这次超时"
            // 译成"上次的错误" —— 超时路径没有回复报文，那个值是残值。
            throw SmbError.fromReadCode(read, credential.anonymous) ?: translate(SmbStage.READ)
        }
        return read
    }

    override fun close() {
        val handle = fh
        fh = 0
        if (handle != 0L && ctx != 0L) Smb2Native.nativeCloseFh(ctx, handle)
        val current = ctx
        ctx = 0
        if (current != 0L) Smb2Native.nativeDestroy(current)
    }

    private fun translate(stage: SmbStage): SmbFailure = SmbError.translate(
        stage,
        Smb2Native.nativeNtStatus(ctx),
        Smb2Native.nativeGetError(ctx),
        credential.anonymous,
    )

    private companion object {
        /** 读超时（秒）。见 [open] 里的说明。 */
        const val READ_TIMEOUT_SECONDS = 3
    }
}
