package com.kivo.cinema.thumbnail

import android.graphics.Bitmap

/**
 * 预览帧读取器的共同契约（v3.1b）。
 *
 * 两个实现：本地索引源走 [ThumbnailReader]（MediaExtractor），
 * 网络/无索引源走 [MpvThumbnailReader]（mpv 第二实例）——接线层只面对本接口。
 *
 * [onUnavailable]：产不出帧时必须给**非空原因**（铁律：不许静默降级），
 * UI 把原因画进预览浮窗的空框里，而不是让用户对着黑框猜。
 */
interface PreviewFrameReader {
    var onThumbnailReady: ((positionUs: Long, bitmap: Bitmap) -> Unit)?
    var onUnavailable: ((reason: String) -> Unit)?

    /** 线程安全；latest-wins：排队中的旧请求被最新请求顶掉。 */
    fun seekTo(positionUs: Long)

    /** 挂载后预热内核（重内核实现用；轻实现默认空操作）。不等首请求才初始化。 */
    fun prewarm() {}

    /** 取证快照（JSON）；解码线程忙时返回带 error 的合法 JSON，不返回空串。 */
    fun evidenceJson(): String

    fun release()
}