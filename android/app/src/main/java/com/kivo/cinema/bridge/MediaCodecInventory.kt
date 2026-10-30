package com.kivo.cinema.bridge

import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.os.Build

/**
 * 职责：把 Java 侧的 MediaCodec 清单摊平成 native 可解析的扁平记录。
 *
 * 为什么这一层必须存在：
 * NDK 的 NdkMediaCodec.h 只提供 `createCodecByName` / `createDecoderByType`，
 * **没有任何枚举或能力查询接口**（NDK r27 已核实）。profile / level / 尺寸上限
 * 只能由 Java 的 MediaCodecInfo 提供。
 *
 * 边界：只做「读平台 API + 摊平字段」，不排序、不判定、不生成 JSON，
 * 那些属于 C++ 核心（见 08-p12-family-tree.md）。
 *
 * 记录格式（TAB 分隔，固定 9 字段，C++ 侧 parse_codec_record 依赖此顺序）：
 * `name \t mime \t isEncoder \t profile \t level \t maxWidth \t maxHeight \t isHardware \t isSoftwareOnly`
 * 未知数值一律填 -1，禁止用 0 冒充。
 */
internal object MediaCodecInventory {

    private const val FIELD_COUNT = 9
    private const val UNKNOWN = -1

    fun collectVideoCodecs(): List<String> {
        val records = ArrayList<String>(128)
        val codecList = MediaCodecList(MediaCodecList.ALL_CODECS)

        for (info in codecList.codecInfos) {
            for (mime in info.supportedTypes) {
                if (!mime.startsWith("video/")) continue
                collectForMime(info, mime, records)
            }
        }
        return records
    }

    private fun collectForMime(
        info: MediaCodecInfo,
        mime: String,
        sink: MutableList<String>
    ) {
        val caps = runCatching { info.getCapabilitiesForType(mime) }.getOrNull()
        val maxWidth = caps?.videoCapabilities?.supportedWidths?.upper ?: UNKNOWN
        val maxHeight = caps?.videoCapabilities?.supportedHeights?.upper ?: UNKNOWN

        val profiles = caps?.profileLevels
        if (profiles.isNullOrEmpty()) {
            sink += buildRecord(info, mime, UNKNOWN, UNKNOWN, maxWidth, maxHeight)
            return
        }
        for (pl in profiles) {
            sink += buildRecord(info, mime, pl.profile, pl.level, maxWidth, maxHeight)
        }
    }

    private fun buildRecord(
        info: MediaCodecInfo,
        mime: String,
        profile: Int,
        level: Int,
        maxWidth: Int,
        maxHeight: Int
    ): String {
        val hardware = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (info.isHardwareAccelerated) 1 else 0
        } else {
            UNKNOWN
        }
        val softwareOnly = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (info.isSoftwareOnly) 1 else 0
        } else {
            UNKNOWN
        }

        val fields = arrayOf(
            info.name,
            mime,
            if (info.isEncoder) "1" else "0",
            profile.toString(),
            level.toString(),
            maxWidth.toString(),
            maxHeight.toString(),
            hardware.toString(),
            softwareOnly.toString()
        )
        check(fields.size == FIELD_COUNT)
        return fields.joinToString("\t")
    }
}
