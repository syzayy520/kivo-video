package com.kivo.cinema.bridge

import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.os.Build
import kotlin.math.roundToInt

/**
 * 职责：把「codec × 分辨率」下的帧率能力摊平成 native 可解析的扁平记录。
 *
 * 为什么需要它（P13-A 的直接动因）：
 * `p12a` 的能力矩阵里所有硬解都报 `max_width = 8192`，那个数字来自
 * `supportedWidths.upper` —— 是芯片的**绝对尺寸上限**，与帧率、与像素吞吐都无关。
 * 拿它判断"4K60 能不能实时解"是错的。真正该问的是
 * `getSupportedFrameRatesFor(w, h)`：同一个 codec 在 1080p 能跑 240fps，
 * 在 4K 可能只到 60fps 甚至 30fps —— **这个数跟分辨率绑定**。
 *
 * 边界：只读平台 API + 摊平，不排序、不判定、不写 JSON。判定在 C++ 侧
 * （见 `56-p13a-design.md` §三）。
 *
 * 记录格式（TAB 分隔，固定 8 字段，C++ 侧 `parse_framerate_record` 依赖此顺序）：
 * `name \t mime \t profile \t width \t height \t sizeSupported \t maxFps \t achievableFps`
 * 未知值一律填 -1；`sizeSupported` 用 1/0/-1。
 */
internal object MediaFramerateInventory {

    private const val FIELD_COUNT = 8
    private const val UNKNOWN_INT = -1
    private const val UNKNOWN_DOUBLE = -1.0

    /**
     * 探测的分辨率。**只放这三个**：
     * 1080p 是当前基线，1440p 是"跳一档看线性度"的中间点，4K 是目标。
     * 加更多只会让输出变长，不会让结论更强。
     */
    private val RESOLUTIONS = listOf(
        1920 to 1080,
        2560 to 1440,
        3840 to 2160,
    )

    fun collect(): List<String> {
        val records = ArrayList<String>(96)
        val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)

        for (info in codecList.codecInfos) {
            if (info.isEncoder) continue
            // 软解在 4K 上没有参考价值（一定跑不动），采集它们只会稀释结论。
            // API 29 以下拿不到 isHardwareAccelerated，那时宁可全采也不猜。
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && !info.isHardwareAccelerated) continue

            for (mime in info.supportedTypes) {
                if (!mime.startsWith("video/")) continue
                val caps = runCatching { info.getCapabilitiesForType(mime) }.getOrNull() ?: continue
                val video = caps.videoCapabilities ?: continue
                collectForMime(info, mime, video, caps.profileLevels, records)
            }
        }
        return records
    }

    private fun collectForMime(
        info: MediaCodecInfo,
        mime: String,
        video: MediaCodecInfo.VideoCapabilities,
        profileLevels: Array<MediaCodecInfo.CodecProfileLevel>?,
        sink: MutableList<String>
    ) {
        // profile 缺失时也要出一条（用 -1），否则某个 codec 会整条消失。
        val profiles: Array<Int> = if (profileLevels.isNullOrEmpty()) {
            intArrayOf(UNKNOWN_INT).toTypedArray()
        } else {
            profileLevels.map { it.profile }.distinct().toTypedArray()
        }

        for (profile in profiles) {
            for ((width, height) in RESOLUTIONS) {
                val sizeOk = runCatching { video.isSizeSupported(width, height) }
                    .map { if (it) 1 else 0 }
                    .getOrDefault(UNKNOWN_INT)

                // 尺寸不支持时这两个 API 会抛（或返回 null）—— 不猜，直接记 -1。
                val maxFps = if (sizeOk == 1) {
                    runCatching { video.getSupportedFrameRatesFor(width, height).upper }
                        .map { it.roundToInt().toDouble() }
                        .getOrDefault(UNKNOWN_DOUBLE)
                } else {
                    UNKNOWN_DOUBLE
                }

                val achievableFps = if (sizeOk == 1) {
                    // 注意方法名是 `getAchievableFrameRatesFor`（带 get）——
                    // 少一个 get 就是编译错误，而它和 getSupportedFrameRatesFor
                    // 只差几个字符，很容易写错。
                    runCatching { video.getAchievableFrameRatesFor(width, height)?.upper }
                        .getOrNull()
                        ?.let { it.roundToInt().toDouble() }
                        ?: UNKNOWN_DOUBLE
                } else {
                    UNKNOWN_DOUBLE
                }

                sink += buildRecord(info.name, mime, profile, width, height, sizeOk, maxFps, achievableFps)
            }
        }
    }

    private fun buildRecord(
        name: String,
        mime: String,
        profile: Int,
        width: Int,
        height: Int,
        sizeOk: Int,
        maxFps: Double,
        achievableFps: Double
    ): String {
        val fields = arrayOf(
            name,
            mime,
            profile.toString(),
            width.toString(),
            height.toString(),
            sizeOk.toString(),
            maxFps.toString(),
            achievableFps.toString()
        )
        check(fields.size == FIELD_COUNT)
        return fields.joinToString("\t")
    }
}
