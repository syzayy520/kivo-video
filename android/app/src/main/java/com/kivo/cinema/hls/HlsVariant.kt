package com.kivo.cinema.hls

/**
 * HLS master playlist 里的一条 **variant**（一个码率档）。
 *
 * ## 为什么单独建包 `com.kivo.cinema.hls`
 * 这里只有**文本解析与纯决策**，不碰 Android、不碰网络、不碰 mpv。
 * 放在 `net` 或 `mpv` 下都会让那两个包反向依赖业务概念；单独一层最干净，
 * 也便于 JVM 单测（310 的核心价值是可复算，不是在真机上试）。
 *
 * @param index 在 master 里出现的顺序（0 起）；[HlsMaster.keepOnly] 用它定位
 * @param uri variant 的相对地址（如 `v5/prog_index.m3u8`，可能带 query）
 * @param bandwidth 声明的峰值带宽（bps）；0 = master 没写
 * @param width 声明宽度；0 = 没写
 * @param height 声明高度；0 = 没写
 * @param codecs 原始 `CODECS` 串（如 `avc1.640020,mp4a.40.2`）；空 = 没写
 */
data class HlsVariant(
    val index: Int,
    val uri: String,
    val bandwidth: Long,
    val width: Int,
    val height: Int,
    val codecs: String,
) {
    /**
     * 视频编码名，取自 `CODECS` 里第一个非音频项。
     *
     * `avc1.640020,mp4a.40.2` → `avc1`；`hvc1.…` → `hvc1`；`av01.…` → `av01`。
     * 给 [com.kivo.cinema.playback.Av1HardwareProbe] 这类能力闸门用。
     */
    val videoCodec: String
        get() = codecs.split(',')
            .map { it.substringBefore('.').trim().lowercase() }
            .firstOrNull { it.isNotEmpty() && !it.startsWith("mp4a") && !it.startsWith("ac-3") && !it.startsWith("ec-3") }
            .orEmpty()
}
