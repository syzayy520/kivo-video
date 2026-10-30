package com.kivo.cinema.bridge

import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.ParcelFileDescriptor
import java.io.File

/**
 * 片源轨道探测（P12-C8）。
 *
 * 职责边界：只回答"这个文件里有没有音轨"，不解码、不读样本。
 *
 * 为什么不放在 native 侧判：C++ 的 `MediaExtractorSource::open_fd(kAudio)` 在两种情况下
 * 报的是同一类错误 ——
 *   - 片源本来就没有音轨（这是**片源属性**，应当显式降级为纯视频）；
 *   - 片源有音轨但平台打不开（这是**故障**，必须中止并带 reason）。
 * 两者在错误字符串上无法可靠区分，而它们的处置方式完全相反。
 * 让 Kotlin 先用平台 API 把"有没有"问清楚，native 侧就只需要处理"有但失败了"。
 */
object MediaTrackProbe {

    private const val AUDIO_MIME_PREFIX = "audio/"

    /**
     * 是否存在音轨。
     *
     * 用 fd 而不是路径：本机 Android 13 上 `AMediaExtractor_setDataSource` 收纯路径
     * 会返回 MALFORMED（P12-B 踩过），Java 的 `setDataSource(String)` 虽然通常没事，
     * 但没必要赌一个已经出过问题的入口。
     *
     * 任何异常都返回 false —— 探不出来就退成纯视频，不该因为"探测失败"而让整轮播不了。
     * 这个降级是安全的：它只影响"要不要接音频时钟"，不影响画面。
     */
    fun hasAudioTrack(file: File): Boolean {
        val descriptor = runCatching {
            ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY)
        }.getOrNull() ?: return false

        val extractor = MediaExtractor()
        return try {
            extractor.setDataSource(descriptor.fileDescriptor)
            (0 until extractor.trackCount).any { index ->
                val mime = extractor.getTrackFormat(index).getString(MediaFormat.KEY_MIME)
                mime?.startsWith(AUDIO_MIME_PREFIX) == true
            }
        } catch (t: Throwable) {
            false
        } finally {
            runCatching { extractor.release() }
            runCatching { descriptor.close() }
        }
    }
}
