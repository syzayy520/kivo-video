#pragma once

#include <algorithm>
#include <cstring>
#include <string_view>

#include "kivo/video/playback_graph/seek_request.hpp"
#include "kivo/video/player_runtime_adapter/adapter_types.hpp"

namespace kivo::video::player_runtime_adapter::commands {

[[nodiscard]] inline playback_graph::OpenRequest to_p7_open_request(
    const AdapterOpenRequest& request) noexcept {
    playback_graph::OpenRequest p7_request{};
    p7_request.source_id = request.source_id;
    p7_request.initial_audio_track_id = request.initial_audio_track_id;
    p7_request.initial_video_track_id = request.initial_video_track_id;
    p7_request.audio_selected = request.audio_selected;
    p7_request.video_selected = request.video_selected;
    return p7_request;
}

[[nodiscard]] inline playback_graph::SeekRequest absolute_seek(std::int64_t position_ms) noexcept {
    playback_graph::SeekRequest request{};
    request.kind = playback_graph::SeekKind::Absolute;
    request.target_timeline_ms = position_ms;
    return request;
}

[[nodiscard]] inline playback_graph::SeekRequest relative_seek(std::int64_t delta_ms) noexcept {
    playback_graph::SeekRequest request{};
    request.kind = playback_graph::SeekKind::Relative;
    request.target_timeline_ms = delta_ms;
    return request;
}

[[nodiscard]] inline playback_graph::PlaybackAspectMode to_p7_aspect(
    AdapterAspectMode mode) noexcept {
    switch (mode) {
        case AdapterAspectMode::Fit:
            return playback_graph::PlaybackAspectMode::Fit;
        case AdapterAspectMode::Fill:
            return playback_graph::PlaybackAspectMode::Fill;
        case AdapterAspectMode::Stretch:
            return playback_graph::PlaybackAspectMode::Stretch;
        case AdapterAspectMode::Original:
            return playback_graph::PlaybackAspectMode::Original;
    }
    return playback_graph::PlaybackAspectMode::Fit;
}

[[nodiscard]] inline playback_graph::PlaybackScaleMode to_p7_scale(
    AdapterScaleMode mode) noexcept {
    switch (mode) {
        case AdapterScaleMode::Auto:
            return playback_graph::PlaybackScaleMode::Auto;
        case AdapterScaleMode::Fit:
            return playback_graph::PlaybackScaleMode::Fit;
        case AdapterScaleMode::Fill:
            return playback_graph::PlaybackScaleMode::Fill;
    }
    return playback_graph::PlaybackScaleMode::Auto;
}

[[nodiscard]] inline playback_graph::PlaybackToneMappingMode to_p7_tone(
    AdapterToneMappingMode mode) noexcept {
    switch (mode) {
        case AdapterToneMappingMode::Auto:
            return playback_graph::PlaybackToneMappingMode::Auto;
        case AdapterToneMappingMode::Off:
            return playback_graph::PlaybackToneMappingMode::Off;
        case AdapterToneMappingMode::Sdr:
            return playback_graph::PlaybackToneMappingMode::Sdr;
        case AdapterToneMappingMode::HdrPassthrough:
            return playback_graph::PlaybackToneMappingMode::HdrPassthrough;
    }
    return playback_graph::PlaybackToneMappingMode::Auto;
}

[[nodiscard]] inline playback_graph::PlaybackDeinterlaceMode to_p7_deinterlace(
    AdapterDeinterlaceMode mode) noexcept {
    switch (mode) {
        case AdapterDeinterlaceMode::Auto:
            return playback_graph::PlaybackDeinterlaceMode::Auto;
        case AdapterDeinterlaceMode::Off:
            return playback_graph::PlaybackDeinterlaceMode::Off;
        case AdapterDeinterlaceMode::On:
            return playback_graph::PlaybackDeinterlaceMode::On;
    }
    return playback_graph::PlaybackDeinterlaceMode::Auto;
}

inline void copy_device_id(playback_graph::AudioDeviceSelectRequest& request,
                           std::string_view device_id) noexcept {
    const auto length = std::min(device_id.size(), sizeof(request.device_id) - 1);
    if (length > 0) {
        std::memcpy(request.device_id, device_id.data(), length);
    }
    request.device_id[length] = '\0';
}

}  // namespace kivo::video::player_runtime_adapter::commands