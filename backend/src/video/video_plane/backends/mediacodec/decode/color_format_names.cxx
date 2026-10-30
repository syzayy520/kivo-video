#include "video/video_plane/backends/mediacodec/decode/color_format_names.hpp"

#include <cstdio>

namespace kivo::video::mediacodec::color_format {
namespace {

[[nodiscard]] std::string hex(std::int32_t value) {
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%X", static_cast<unsigned>(value));
    return std::string(buffer);
}

}  // namespace

std::string format_name(std::int32_t format) {
    switch (format) {
        case kYuv420Planar:
            return "yuv420p";
        case kYuv420SemiPlanar:
            return "nv12";
        case kYuvP010:
            return "yuvp010";
        case kSurface:
            return "surface";
        case kYuv420Flexible:
            return "yuv420flexible";
        case kVendorQcomTile:
            return "qcom_private";
        default:
            break;
    }
    return "unknown(" + hex(format) + ")";
}

bool is_vendor_private(std::int32_t format) noexcept {
    // 0x7FA30C00–0x7FA30CFF 是高通自用区；其他厂商各有各的段，暂不枚举。
    return (format & 0xFFFFFF00) == 0x7FA30C00;
}

std::int32_t format_bit_depth(std::int32_t format) {
    switch (format) {
        case kYuvP010:
            return 10;
        case kYuv420Planar:
        case kYuv420SemiPlanar:
            return 8;
        default:
            // surface / flexible / 未知值：**不猜**。
            // 把 surface 当 8bit 会让"看不到"变成"确认是 8bit"，那是一句假话。
            return 0;
    }
}

}  // namespace kivo::video::mediacodec::color_format
