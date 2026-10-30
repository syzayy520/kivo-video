#pragma once

#include <cstdint>
#include <string_view>

namespace kivo::video::mediacodec {

/// 解封装时要选哪一类轨道。
///
/// 视频与音频共用同一套 AMediaExtractor 生命周期（打开 fd、选轨、取包、seek），
/// 差别只在"按哪个 MIME 前缀挑"。把这点差异做成一个参数，
/// 而不是复制两个几乎一样、迟早会漂移的解封装器。
enum class TrackKind : std::int32_t {
    kVideo = 0,
    kAudio = 1,
};

/// MIME 是否属于指定种类。前缀判断是 Android 的既定约定（"video/..."、"audio/..."）。
[[nodiscard]] constexpr bool mime_matches_kind(std::string_view mime, TrackKind kind) noexcept {
    const std::string_view prefix = (kind == TrackKind::kAudio) ? "audio/" : "video/";
    return mime.size() >= prefix.size() && mime.compare(0, prefix.size(), prefix) == 0;
}

[[nodiscard]] constexpr std::string_view track_kind_name(TrackKind kind) noexcept {
    return (kind == TrackKind::kAudio) ? "audio" : "video";
}

}  // namespace kivo::video::mediacodec
