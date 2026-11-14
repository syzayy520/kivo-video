package com.kivo.cinema.net

/**
 * 273 v17：读侧装饰器 —— **缓存优先，缺则委托**。
 *
 * 【它是什么】窗口缓存（[OriginWindowCache]）与源站读者之间的中介：
 * 请求落在缓存里就本地供，落在外面就交给源站读者。
 *
 * 【为什么 size() 要先问"源"】
 * 代理写响应头需要片源总长。**这个总长在读者自己那一跳的响应头里就有**
 * （`Content-Range` 的 `/total`）⇒ 先问源（已缓存则零等待），
 * 源还不知道才问委托读者 —— 委托读者会等它自己那一跳的表头，
 * **不额外往返**（旧实现是先单独探一次总长，真机 ≈300 ms）。
 *
 * 【为什么命中一次之后就不再查缓存】
 * 缓存是**加速器**，不是数据源。命中之后内核继续往下一个偏移读，
 * 那时窗口已经盖不住了 —— 若此时窗口才落地，读侧会在半途切回缓存，
 * 委托读者的游标就对不上 ⇒ 白白触发一次跳读（重开连接）。
 * 所以：**第一次落空就固定走委托**，语义稳定，代价只有"没用上缓存"。
 */
class CachedReader(
    /** 这个读者自己的起点（代理按连接派发时给的那一段的起点）。 */
    private val startOffset: Long,
    private val openDelegate: (Long) -> ByteSource,
    private val cache: OriginWindowCache,
    /** 源已经知道的总长（-1 = 还不知道）。 */
    private val knownSize: () -> Long,
) : ByteSource {

    private var delegate: ByteSource? = null

    /** 还在试缓存吗（第一次落空就否）。 */
    private var cacheMode = true

    override fun size(): Long {
        knownSize().takeIf { it > 0 }?.let { return it }
        val current = delegate ?: openDelegate(startOffset).also { delegate = it }
        return current.size()
    }

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        if (len <= 0 || offset < 0) return 0
        if (cacheMode) {
            val cached = cache.readAt(offset, buf, off, len)
            if (cached > 0) return cached
            cacheMode = false
        }
        val current = delegate ?: openDelegate(offset).also { delegate = it }
        return current.readAt(offset, buf, off, len)
    }

    override fun close() {
        delegate?.let { runCatching { it.close() } }
        delegate = null
    }
}
