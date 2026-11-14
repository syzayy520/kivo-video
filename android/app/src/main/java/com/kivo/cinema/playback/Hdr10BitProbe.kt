package com.kivo.cinema.playback

import android.graphics.PixelFormat
import android.media.ImageReader

/**
 * 278-P2 · **10bit buffer 能力探测**。
 *
 * ## 为什么必须有这一步
 *
 * 真 HDR 输出的第 ① 层（Surface buffer = 10bit）不是"设个 flag"就行的：
 * 我们让 mpv 用 `--egl-output-format=rgb10_a2` 选 10bit 的 EGL config，
 * mpv 随后用这个 config 的 native visual id（0x2b = RGBA_1010102）去
 * `ANativeWindow_setBuffersGeometry`，并由 EGL 在 swap 时向 gralloc 要 10bit buffer。
 * 在**不支持 10bit RGBA 的 gralloc** 上，这一步会**卡死**
 * （真机 K40 实测：内核日志停在 EGLConfig dump，VO 初始化再不返回）。
 *
 * ⇒ 必须在**让 mpv 用 10bit 之前**先问一句"这台设备到底行不行"。
 *
 * ## 怎么问
 *
 * 用 `ImageReader`（公开 API）开一个 10bit 的 buffer queue：
 * `ImageReader.newInstance(w, h, ImageFormat.RGBA_1010102, 2)`。
 * 它走的是**与 EGL/ANativeWindow 同一个 gralloc 分配路径**，
 * 而**失败是抛异常、不是挂死** ⇒ 可以安全地放在起播关键路径上。
 *
 * 【为什么不用 `HardwareBuffer`】它是 `@hide` API，不在公开 SDK 里（编译不过）。
 * 【为什么不用 `Surface.lockCanvas`】那走的是 CPU 画布路径，与 EGL 的
 * buffer 分配路径不同 ⇒ 探测结果不能代表 EGL 能不能拿到 10bit buffer。
 *
 * ## 结果怎么用
 *
 * 探测结果**进程内缓存**（gralloc 能力不会变，只探一次）：
 * · 支持 ⇒ 走真 HDR 路径（10bit + PQ dataspace + EGL PQ 标注）；
 * · 不支持 ⇒ 留在 SDR 路径，并在诊断日志里留痕（**不许静默降级**）。
 */
internal object Hdr10BitProbe {

    @Volatile
    private var cached: Boolean? = null

    /**
     * 这台设备的 gralloc 能不能分配 10bit RGBA buffer。
     *
     * @return true = 支持（可真 HDR 输出）；false = 不支持（留在 SDR）
     */
    fun supports10Bit(): Boolean {
        cached?.let { return it }
        val result = try {
            // PixelFormat.RGBA_1010102 = 0x2B（= gralloc 的 R10G10B10A2_UNORM）。
            // ImageFormat 里没有这个常量，但 ImageReader 接受 PixelFormat 的值。
            // 尺寸取小的：能力与尺寸无关，分配更快。
            val reader = ImageReader.newInstance(16, 16, PixelFormat.RGBA_1010102, 2)
            reader.close()
            true
        } catch (_: Throwable) {
            false
        }
        cached = result
        // 【留痕放在探测者自己身上，而且只在真正探测的这一下】
        // 280 之前这句在 [HdrOutputSwitch] 里 ⇒ 每调一次 isEnabled 就打一遍，
        // 真机实测起播时同一条降级原因刷了 3 次（Activity 一次 + 内核侧一次 +
        // Surface 重建又一次）。降级原因必须留痕，但**一次就够**。
        if (!result) {
            com.kivo.cinema.net.NetDiag.log(
                "[hdr] 真 HDR 输出不可用：设备的 gralloc 不支持 10bit buffer" +
                    "（10bit 是 PQ 的必要条件）⇒ 留在 SDR 路径"
            )
        }
        return result
    }

    /** 清缓存（仅测试用）。 */
    fun reset() {
        cached = null
    }
}
