package com.kivo.cinema.thumbnail

/** 一次 seek 的不可变身份；positionUs 是请求目标，不是解码器的关键帧 PTS。 */
class PreviewRequest internal constructor(val id: Long, val positionUs: Long)

/** 每个 reader 独占的请求代际闸门；无 Android/解码/调度依赖。 */
internal class PreviewRequestGate {
    private var nextId = 0L
    private var current: PreviewRequest? = null

    @Synchronized
    fun next(positionUs: Long): PreviewRequest =
        PreviewRequest(++nextId, positionUs).also { current = it }

    @Synchronized
    fun invalidate() {
        current = null
    }

    // 按对象身份校验，防同位置重试或另一个 reader 的相同 id 误命中。
    @Synchronized
    fun isCurrent(request: PreviewRequest): Boolean = current === request
}
