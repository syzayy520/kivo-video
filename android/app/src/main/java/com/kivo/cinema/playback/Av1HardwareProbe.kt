package com.kivo.cinema.playback

import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.os.Build

/**
 * 301 · **AV1 硬解能力探测**。
 *
 * ## 为什么需要它
 *
 * "这个片源能不能播"不只取决于**码率**，还取决于 **codec × 分辨率 × 设备硬解能力**。
 * 301 实测（K40 / SM8250）：
 *
 * | 片源 | 结果 |
 * | --- | --- |
 * | 1080p AV1 10bit（真实电影） | ✅ **60.44 fps**（dav1d 软解够用） |
 * | **4K AV1 10bit（真实剧集）** | ❌ **10.87 fps**（dav1d 9 线程打满，CPU 766%） |
 * | 4K HEVC HDR（真实内容） | ✅ 59.47 fps（硬解） |
 *
 * ⇒ 4K 以上的 AV1，**没有硬解就等于不能播**。这条判断必须能被选源和起播提示复用。
 *
 * ## 为什么必须问系统，不能按 SoC 型号猜
 *
 * 同代 SoC 的 AV1 支持不一致（8 Gen 1 无、8 Gen 2 有），厂商也可能只给部分档位。
 * `MediaCodecList` 是设备自己报的事实，比任何型号表可靠。
 *
 * ## 软件解码器不算
 *
 * `c2.android.av1.decoder` 之类的**软解**也会出现在列表里，但它正是
 * "4K 跑不动"的那条路径 ⇒ 必须用 `isHardwareAccelerated` 过滤掉。
 */
object Av1HardwareProbe {

    private const val MIME_AV1 = "video/av01"

    @Volatile
    private var cached: Boolean? = null

    /** 本机是否有 AV1 **硬件**解码器（进程内缓存：设备能力不会变）。 */
    fun hasHardwareAv1(): Boolean {
        cached?.let { return it }
        val v = runCatching { probe() }.getOrDefault(false)
        cached = v
        return v
    }

    /**
     * 该 codec+分辨率组合在本机是否**大概率无法实时播放**。
     *
     * 【只拦已实测确认跑不动的组合】4K+ 的 AV1 且无硬解。
     * 不做其它猜测 —— 猜错会让本来能播的片源被无谓降权（那是另一种静默降级）。
     *
     * @param codec Emby 的 `MediaStreams[].Codec`（"av1"）或 mpv 的 codec 名
     * @param height 视频高度（1080 / 2160）；≤0 表示未知 ⇒ 不判
     * @param av1Hw 本机是否有 AV1 硬解；默认现探（测试可注入，避免依赖设备）
     */
    fun isLikelyUnplayable(
        codec: String?,
        height: Int,
        av1Hw: Boolean = hasHardwareAv1(),
    ): Boolean {
        if (codec == null) return false
        val isAv1 = codec.equals("av1", true) || codec.equals("av01", true)
        if (!isAv1) return false
        // 1080p AV1 软解实测 60.44 fps ⇒ 可播，不拦。
        if (height in 1..1080) return false
        // 高度未知时保守放行：宁可让它试，也不要在没有证据时替用户拒绝。
        if (height <= 0) return false
        return !av1Hw
    }

    private fun probe(): Boolean {
        val list = MediaCodecList(MediaCodecList.ALL_CODECS)
        for (info in list.codecInfos) {
            if (info.isEncoder) continue
            if (!isHardwareDecoder(info)) continue
            if (info.supportedTypes.any { it.equals(MIME_AV1, ignoreCase = true) }) return true
        }
        return false
    }

    /**
     * 是否硬件解码器。
     *
     * `isHardwareAccelerated` 是 API 29 才有的；minSdk 是 26，
     * 旧系统按命名前缀判断（`OMX.google.` / `c2.android.` 是 AOSP 软解）。
     */
    private fun isHardwareDecoder(info: MediaCodecInfo): Boolean {
        if (Build.VERSION.SDK_INT >= 29) return info.isHardwareAccelerated
        val n = info.name.lowercase()
        return !n.startsWith("omx.google.") && !n.startsWith("c2.android.")
    }
}
