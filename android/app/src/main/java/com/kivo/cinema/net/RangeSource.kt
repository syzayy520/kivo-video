package com.kivo.cinema.net

import android.os.ParcelFileDescriptor

/**
 * "能变成一个可随机读的 fd"（243 D3）。播放内核只认 fd，所以 SMB / WebDAV / 本地
 * 都必须收敛到这一个形态，再由 [NetworkPlaybackSession] 交给 FFmpeg 或 MediaExtractor。
 *
 * 实现方负责两件事：把字节读出来、把失败原因写进 [failureReason]（非空且用户可读）。
 * [openDescriptor] 可能被调用多次（提取器 + 解码器各要一个 fd），每次都要独立可读。
 */
interface RangeSource : AutoCloseable {

    val size: Long

    /** 最近一次失败原因；空白表示没有失败。 */
    val failureReason: String

    /**
     * 当前是否正在"等网络回来"（读取失败后的重试窗口内）。
     *
     * 与 [failureReason] 的区别：stalled 是**暂时**状态，正常会自己恢复，
     * 界面应该说"正在重连"；failureReason 一旦非空就是终态，要说"失败"。
     * 缺省 false，不需要这个语义的实现不用管它。
     */
    val stalled: Boolean get() = false

    /**
     * 准备阶段：探测大小、建连、校验服务器能力。
     * 失败时抛出，并把原因写进 [failureReason]。已就绪的实现可以是空操作。
     */
    fun open()

    fun openDescriptor(): ParcelFileDescriptor

    /** 打断阻塞中的读取。可在任意线程调用，必须幂等。 */
    fun cancel()
}
