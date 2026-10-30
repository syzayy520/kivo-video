#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// `AMediaFormat` 颜色键的取值表。
///
/// **数值不是背出来的，是从本机 Android SDK 反查的**：
/// `javap -constants -classpath $ANDROID_HOME/platforms/android-35/android.jar \
///      android.media.MediaFormat`
///
/// 为什么要这么郑重：这些常量 NDK 不导出（`AMEDIAFORMAT_KEY_COLOR_STANDARD` 只在 Java 侧），
/// 手写错一个数字不会有任何编译错误，只会让判定与证据一起说瞎话 ——
/// P12-E1 就在 `color-format` 上踩过一次（以为 Surface 是 0x7F000789，实际是厂商私有值）。
///
/// 三条与直觉不符的实测值，别"顺手修正"回去：
/// 1. `BT601_NTSC = 4`，不是 3 —— 3 在 `color-standard` 上根本没有定义；
/// 2. `SDR_VIDEO = 3`（`color-transfer`），不是 `SMPTE_240M`；
///    E1 在无标记片源上看到的 `1/3/2` 就是 BT709 / SDR_VIDEO / LIMITED 这组默认值；
/// 3. **`MediaFormat` 里没有 BT2020_CONSTANT_LUMINANCE**（那是 `DataSpace` 的 `STANDARD_*`）。
///    所以 HDR 判据只认 `standard == 6`，不认 7 —— 认 7 等于认一个在
///    `color-standard` 键上不存在的取值。
namespace color_key {

constexpr std::int32_t kStandardBt709 = 1;
constexpr std::int32_t kStandardBt601Pal = 2;
constexpr std::int32_t kStandardBt601Ntsc = 4;
constexpr std::int32_t kStandardBt2020 = 6;

constexpr std::int32_t kTransferLinear = 1;
constexpr std::int32_t kTransferSdrVideo = 3;
constexpr std::int32_t kTransferSt2084 = 6;  ///< PQ
constexpr std::int32_t kTransferHlg = 7;

constexpr std::int32_t kRangeFull = 1;
constexpr std::int32_t kRangeLimited = 2;

/// 键名字面量。`AMEDIAFORMAT_KEY_COLOR_*` 是 API 28 才导出的常量，而 minSdk = 26；
/// 键名本身自 API 24 稳定（P12-E1 已按同样理由用字面量读 `color-standard`）。
constexpr const char* kKeyColorStandard = "color-standard";
constexpr const char* kKeyColorTransfer = "color-transfer";
constexpr const char* kKeyColorRange = "color-range";
constexpr const char* kKeyHdrStaticInfo = "hdr-static-info";
/// P12-E4：请求解码器把输出转成某个 transfer（`MediaFormat.KEY_COLOR_TRANSFER_REQUEST`，
/// 已由 `javap android.media.MediaFormat` 核对为 `"color-transfer-request"`）。
/// 与 `color-format` 是两条独立的键 —— 前者是"像素布局"，后者是"传递函数"。
constexpr const char* kKeyColorTransferRequest = "color-transfer-request";

[[nodiscard]] inline const char* standard_name(std::int32_t value) noexcept {
    switch (value) {
        case kStandardBt709:
            return "bt709";
        case kStandardBt601Pal:
            return "bt601_pal";
        case kStandardBt601Ntsc:
            return "bt601_ntsc";
        case kStandardBt2020:
            return "bt2020";
        case -1:
            return "unknown(-1)";
        default:
            return "unknown";
    }
}

[[nodiscard]] inline const char* transfer_name(std::int32_t value) noexcept {
    switch (value) {
        case kTransferLinear:
            return "linear";
        case kTransferSdrVideo:
            return "sdr_video";
        case kTransferSt2084:
            return "st2084(pq)";
        case kTransferHlg:
            return "hlg";
        case -1:
            return "unknown(-1)";
        default:
            return "unknown";
    }
}

[[nodiscard]] inline const char* range_name(std::int32_t value) noexcept {
    switch (value) {
        case kRangeFull:
            return "full";
        case kRangeLimited:
            return "limited";
        case -1:
            return "unknown(-1)";
        default:
            return "unknown";
    }
}

}  // namespace color_key

}  // namespace kivo::video::mediacodec
