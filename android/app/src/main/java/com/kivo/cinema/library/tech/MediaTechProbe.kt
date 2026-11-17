package com.kivo.cinema.library.tech

import android.media.MediaExtractor
import android.media.MediaFormat

/**
 * 片源技术探测（D1）：一条 `MediaExtractor` 只读轨道格式，不建解码器、不读样本。
 *
 * ## 为什么只读 MediaExtractor 的轨道格式
 *
 * 详情页只需要"静态标签"（4K？HEVC？10bit？HDR10？码率？），
 * 这些全在 `MediaFormat` 里；建 `MediaCodec` 会多花几百毫秒并占用硬件解码器，
 * 在详情页这种"瞄一眼就走"的场景不划算。
 *
 * ## 已知边界（不夸大）
 *
 * · **位深只能由 profile 推**（P12-E1 实测：`color-format` 对 8/10bit 返回同一私有值
 *   `0x7FA30C06` ⇒ 平台不提供内容位深）。profile 缺失时返回 null，不默认 8。
 * · **MKV 可能慢。** 平台 `MediaExtractor` 不解析 MKV 的 SeekHead→Cues（P12 已踩），
 *   缺索引的大文件会线性扫；调用方**必须**放在 IO 线程（本函数是同步阻塞的）。
 * · **网络 SMB / HTTP 路径**未纳入：`setDataSource(String)` 只处理本地路径，
 *   走网络源时调用方应先落成本地 fd（那是 D4 之后的事）。
 */
object MediaTechProbe {

    private const val VIDEO_PREFIX = "video/"
    private const val AUDIO_PREFIX = "audio/"

    /** 常见视频 mime → 展示简称。表外一律 null（不猜）。 */
    private val VIDEO_CODECS = mapOf(
        "video/hevc" to "HEVC",
        "video/avc" to "AVC",
        "video/av01" to "AV1",
        "video/mp4v-es" to "MPEG4",
    )

    /** 常见音频 mime → 展示简称。表外保留原始 mime（见 [audioOf]）。 */
    private val AUDIO_CODECS = mapOf(
        "audio/mp4a-latm" to "AAC",
        "audio/ac3" to "AC3",
        "audio/eac3" to "EAC3",
        "audio/dts" to "DTS",
        "audio/opus" to "OPUS",
        "audio/flac" to "FLAC",
    )

    /**
     * `MediaFormat.KEY_COLOR_TRANSFER` 的取值（android/media/MediaFormat.java）。
     * 只把**明确 SDR** 的两个值认成 [HdrKind.None]；0（unspecified）与其它值一律
     * [HdrKind.Unknown] —— "没写"不等于"是 SDR"。
     */
    private const val TRANSFER_LINEAR = 1
    private const val TRANSFER_SDR_VIDEO = 3
    private const val TRANSFER_ST2084 = 6
    private const val TRANSFER_HLG = 7

    /** `CodecProfileLevel` 里能确定 10bit 的 profile（字面量，避免引用高 API 常量）。 */
    private const val HEVC_PROFILE_MAIN10 = 0x02
    private const val HEVC_PROFILE_MAIN10_HDR10 = 0x2000
    private const val HEVC_PROFILE_MAIN10_HDR10_PLUS = 0x2001
    private const val AVC_PROFILE_HIGH10 = 0x10

    /** 杜比视界的 codec 名（`MediaFormat` 的 mime 或 `codecs-string` 里出现）。 */
    private val DOLBY_VISION_TAGS = listOf("dvhe", "dvh1", "dav1")

    /** `MediaFormat.KEY_CODECS_STRING`（API 29 起才有，故用字面量）。 */
    private const val KEY_CODECS_STRING = "codecs-string"

    /**
     * 同步阻塞，**必须在 IO 线程调用**。
     *
     * @return 探测结果；文件打不开、没有视频轨、或任何异常 → null（不造假）。
     */
    fun probe(path: String): MediaTechInfo? {
        if (path.isBlank()) return null
        return runCatching { probeOrNull(path) }.getOrNull()
    }

    private fun probeOrNull(path: String): MediaTechInfo? {
        val extractor = MediaExtractor()
        try {
            extractor.setDataSource(path)
            val formats = (0 until extractor.trackCount).map { extractor.getTrackFormat(it) }

            val video = formats.firstOrNull { mimeOf(it)?.startsWith(VIDEO_PREFIX) == true }
                ?: return null
            val videoMime = mimeOf(video)
            val videoCodec = videoMime?.let { VIDEO_CODECS[it] }

            return MediaTechInfo(
                width = intOrNull(video, MediaFormat.KEY_WIDTH) ?: 0,
                height = intOrNull(video, MediaFormat.KEY_HEIGHT) ?: 0,
                videoCodec = videoCodec,
                bitDepth = bitDepthOf(videoCodec, intOrNull(video, MediaFormat.KEY_PROFILE)),
                bitrateKbps = bitrateKbpsOf(video),
                hdr = hdrOf(videoMime, video),
                audio = formats.filter { mimeOf(it)?.startsWith(AUDIO_PREFIX) == true }
                    .map { audioOf(it) },
            )
        } finally {
            extractor.release()
        }
    }

    /**
     * 位深只认 profile：HEVC Main10 系 / AVC High10。
     * 其余（含 8bit profile、以及 profile 键缺失）一律 null —— 默认 8 会把 10bit 片源
     * 标成 8bit，这类误报比"不显示"更糟。
     */
    private fun bitDepthOf(codec: String?, profile: Int?): Int? {
        if (profile == null) return null
        return when (codec) {
            "HEVC" -> when (profile) {
                HEVC_PROFILE_MAIN10, HEVC_PROFILE_MAIN10_HDR10,
                HEVC_PROFILE_MAIN10_HDR10_PLUS -> 10
                else -> null
            }
            "AVC" -> if (profile == AVC_PROFILE_HIGH10) 10 else null
            else -> null
        }
    }

    /**
     * HDR 判定顺序：杜比视界优先（它自己的 mime 最可信），再看 transfer。
     *
     * 杜比视界**不从 transfer 推**：DV 的 base layer 常报 6（PQ），
     * 光看 transfer 会把 DV 判成 HDR10。只有 mime/codecs 点名才认。
     */
    private fun hdrOf(mime: String?, format: MediaFormat): HdrKind {
        val codecs = stringOrNull(format, KEY_CODECS_STRING)
        val namesDolby = DOLBY_VISION_TAGS.any { tag ->
            mime?.contains(tag, ignoreCase = true) == true ||
                codecs?.contains(tag, ignoreCase = true) == true
        }
        if (namesDolby) return HdrKind.DolbyVision

        return when (val transfer = intOrNull(format, MediaFormat.KEY_COLOR_TRANSFER)) {
            TRANSFER_ST2084 -> HdrKind.HDR10
            TRANSFER_HLG -> HdrKind.HLG
            TRANSFER_LINEAR, TRANSFER_SDR_VIDEO -> HdrKind.None
            null -> HdrKind.Unknown
            else -> HdrKind.Unknown // 0=unspecified 及未覆盖的取值：不冒充 SDR
        }
    }

    /** `KEY_BIT_RATE` 是 bit/s；≤0 视为缺失。 */
    private fun bitrateKbpsOf(format: MediaFormat): Int? {
        val bps = intOrNull(format, MediaFormat.KEY_BIT_RATE) ?: return null
        if (bps <= 0) return null
        return bps / 1000
    }

    private fun audioOf(format: MediaFormat): AudioTrackInfo {
        val mime = mimeOf(format)
        val language = stringOrNull(format, MediaFormat.KEY_LANGUAGE)
            ?.trim()
            ?.takeIf { it.isNotEmpty() && !it.equals("und", ignoreCase = true) }
        return AudioTrackInfo(
            codec = mime?.let { AUDIO_CODECS[it] } ?: mime ?: "unknown",
            // MediaFormat 没有声道键时返回 0：不推断"至少是立体声"。
            channels = intOrNull(format, MediaFormat.KEY_CHANNEL_COUNT) ?: 0,
            language = language,
        )
    }

    private fun mimeOf(format: MediaFormat): String? = stringOrNull(format, MediaFormat.KEY_MIME)

    /** 键缺失、类型不符（如某些容器把 profile 写成 String）都退成 null，不抛。 */
    private fun intOrNull(format: MediaFormat, key: String): Int? = runCatching {
        if (format.containsKey(key)) format.getInteger(key) else null
    }.getOrNull()

    private fun stringOrNull(format: MediaFormat, key: String): String? = runCatching {
        if (format.containsKey(key)) format.getString(key) else null
    }.getOrNull()
}