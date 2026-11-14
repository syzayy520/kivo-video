package com.kivo.cinema.net

/**
 * 能按"连接"派发**独立读者**的字节源。
 *
 * ## 为什么单游标的 [ByteSource] 不够
 * mpv 内核走 FFmpeg 的 `http` 协议，**每次 seek 都会新开一条连接**，
 * 而且新旧连接会短暂重叠：旧连接的读线程还没停，新连接已经带着新的 `Range` 到了。
 * 单游标的源只有一个流，两个读者会互相把对方顶掉（旧连接被拽回旧位置，新连接被拽走）。
 *
 * 真机日志（273）里能直接看到这个形态：起播阶段内核连开 4 条连接
 * （读头 → 读尾 cues → 读尾 → 回到头部），每次都对应源站的一次重新建连。
 *
 * ## 契约
 *  · [openReader] 返回**互相独立**的读者：各自的连接、各自的位置、各自的缓冲。
 *  · **谁创建谁拥有**：调用方负责关闭返回的读者。
 */
interface RangeReaderSource {

    /** 片源总长（字节）。 */
    fun size(): Long

    /** 开一个从 [offset] 开始的独立读者。 */
    fun openReader(offset: Long): ByteSource
}
