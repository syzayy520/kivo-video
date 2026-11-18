package com.kivo.cinema.net

/**
 * 354 · **按需打开**的本地文件源。
 *
 * ## 为什么需要它
 *
 * 本地文件默认直连 mpv（`loadfile <path>`），那条路根本不需要 [ByteSource]。
 * 但 `debug.kivo.inputMode=proxy`（对照实验）或直连预检失败时，起播会回退到
 * loopback 代理 —— 那时才真的需要一个源。
 *
 * 直接用 [FileByteSource] 会**无条件打开一个 fd**：直连成立时这个 fd 从头到尾没人读，
 * 白占一个句柄。本类把打开推迟到第一次真正读，直连路径零成本。
 *
 * ## 生命周期
 *
 * 谁创建谁关闭。会话（`MpvPlaybackSession`）创建它 ⇒ 会话释放时关它；
 * 未打开过的实例 `close()` 是空操作，不会反过来创建句柄。
 */
internal class LazyFileByteSource(private val path: String) : ByteSource {

    private val holder = lazy(LazyThreadSafetyMode.SYNCHRONIZED) { FileByteSource(path) }

    override fun size(): Long = holder.value.size()

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int =
        holder.value.readAt(offset, buf, off, len)

    override fun close() {
        // 只在已经打开过时才关 —— 访问 holder.value 会触发打开，那就本末倒置了。
        if (holder.isInitialized()) runCatching { holder.value.close() }
    }
}
