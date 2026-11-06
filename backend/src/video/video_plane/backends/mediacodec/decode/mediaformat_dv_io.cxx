#include "video/video_plane/backends/mediacodec/decode/mediaformat_dv_io.hpp"

#include <cstddef>

namespace kivo::video::mediacodec {
namespace {

// 键名字面量，不用 NDK 常量：`AMEDIAFORMAT_KEY_PROFILE` / `AMEDIAFORMAT_KEY_LEVEL`
// 是 API 28 才导出的符号，而 minSdk = 26 —— 编译器直接标 unavailable（首版构建
// 就撞了）。键名用字面量是这个工程对 API 28 符号的既定处理（坑表：`"csd-0"` 同款）。
// 出处：K40 实测轨道格式里的键名（2026-11-06），
// 见 docs/evidence/p210/p210-track-dump-dv81.json 的 track_format_dump。
constexpr const char* kKeyProfile = "profile";
constexpr const char* kKeyLevel = "level";
constexpr const char* kKeyBlCompatibilityId = "bl_compatibility_id";
constexpr const char* kKeyCsd0 = "csd-0";

}  // namespace

DvTrackKeys read_dv_track_keys(const AMediaFormat* track_format) {
    DvTrackKeys keys;
    if (track_format == nullptr) {
        return keys;
    }
    auto* format = const_cast<AMediaFormat*>(track_format);

    std::int32_t value = -1;
    if (AMediaFormat_getInt32(format, kKeyProfile, &value)) {
        keys.profile_key = value;
    }
    if (AMediaFormat_getInt32(format, kKeyLevel, &value)) {
        keys.level_key = value;
    }
    if (AMediaFormat_getInt32(format, kKeyBlCompatibilityId, &value)) {
        keys.bl_compatibility_id = value;
    }
    return keys;
}

bool read_csd0_bytes(const AMediaFormat* track_format, std::vector<std::uint8_t>& out) {
    out.clear();
    if (track_format == nullptr) {
        return false;
    }
    void* data = nullptr;
    std::size_t size = 0;
    if (!AMediaFormat_getBuffer(const_cast<AMediaFormat*>(track_format), kKeyCsd0, &data, &size) ||
        data == nullptr || size == 0) {
        return false;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    out.assign(bytes, bytes + size);
    return true;
}

}  // namespace kivo::video::mediacodec
