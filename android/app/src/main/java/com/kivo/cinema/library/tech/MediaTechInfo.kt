package com.kivo.cinema.library.tech

/**
 * 片源技术信息模型（D1）。
 *
 * ## 设计取舍（为什么是这个形状）
 *
 * · **可空而不是有默认值。** `null` 表示"平台上确实拿不到"，不是"8bit / SDR"。
 *   历史上把缺失当默认值造成过误报（P12-E1：位深"8bit 与 10bit 片源同值"被当成可判定），
 *   ⇒ 本模型宁可能空，也不给"看起来合理"的默认值。
 * · **`hdr` 的 Unknown 与 None 是两个语义。** `None` = 确认是 SDR（transfer 键明确为
 *   SDR_VIDEO/LINEAR）；`Unknown` = 没拿到键或拿到不认识的值。UI 只认 `HDR10/HLG/
 *   DolbyVision` 三个值画徽章，所以两者在展示上等价，但在证据/诊断上不能混。
 * · **`audio` 是列表不是单轨。** 同一文件多语言音轨很常见；只取第一条会丢掉"有中文轨"
 *   这种用户关心的信息。
 *
 * ## 数值口径来源
 *
 * · 编码/位深/HDR 键位与取值：P12-E1…E4 真机实测（✅ 见
 *   `docs/evidence/p12e*`）——`color-transfer=6` 对应 HDR10 片源、`KEY_PROFILE` 可读。
 * · 位深**只能**由 `KEY_PROFILE` 推：P12-E1 结论是 `color-format` 八/十 bit 同值，
 *   平台不提供"内容位深"这个独立字段（✅ 实测：`0x7FA30C06` 两者一致）。
 */
enum class HdrKind {
    /** 确认 SDR（transfer 键明确为 SDR_VIDEO / LINEAR）。 */
    None,

    /** PQ（ST2084）传递函数，HDR10 / HDR10+ 的基础。 */
    HDR10,

    /** HLG 传递函数。 */
    HLG,

    /** 杜比视界：仅当 mime / codecs 字符串点名 `dvhe` / `dvh1` / `dav1` 才判。 */
    DolbyVision,

    /** 拿不到或无法判定（**不是** SDR）。 */
    Unknown,
}

/**
 * 一条音轨。
 *
 * @param codec 简称（`AAC`/`AC3`/`EAC3`/`DTS`/`OPUS`/`FLAC`）；不认识的 mime **原样保留**
 *   （如 `audio/vorbis`），不做猜测性归类。
 * @param channels 声道数；`KEY_CHANNEL_COUNT` 缺失时为 0（MediaFormat 没有该键，不推断 2）。
 * @param language ISO 639-2 语言码；缺失或 `und` 时为 null。
 */
data class AudioTrackInfo(
    val codec: String,
    val channels: Int,
    val language: String?,
)

/**
 * 一个片源的技术画像。
 *
 * @param width 视频轨宽（像素）；拿不到 0。
 * @param height 视频轨高（像素）；拿不到 0。
 * @param videoCodec `HEVC` / `AVC` / `AV1` / `MPEG4`；其它编码为 null（不猜）。
 * @param bitDepth 8 / 10；**无法判定时为 null**，绝不默认 8。
 * @param bitrateKbps 视频轨 `KEY_BIT_RATE` / 1000；缺失或 ≤0 时为 null。
 * @param hdr 见 [HdrKind]。
 * @param audio 按轨道序号排列；可能为空（无音轨的片源是合法状态）。
 */
data class MediaTechInfo(
    val width: Int,
    val height: Int,
    val videoCodec: String?,
    val bitDepth: Int?,
    val bitrateKbps: Int?,
    val hdr: HdrKind,
    val audio: List<AudioTrackInfo>,
) {
    /** 4K 及以上（DCI 4K 4096 与 UHD 3840 都算）。 */
    val is4K: Boolean get() = width >= 3840 || height >= 2160

    /** 只在确认 HDR 时才有意义（Unknown 不冒充）。 */
    val hasHdr: Boolean get() = hdr == HdrKind.HDR10 || hdr == HdrKind.HLG ||
        hdr == HdrKind.DolbyVision
}