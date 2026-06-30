#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class PlaybackAspectMode {
    Fit,
    Fill,
    Stretch,
    Original
};

enum class PlaybackScaleMode {
    Auto,
    Fit,
    Fill
};

enum class PlaybackToneMappingMode {
    Auto,
    Off,
    Sdr,
    HdrPassthrough
};

enum class PlaybackDeinterlaceMode {
    Auto,
    Off,
    On
};

struct PlaybackSettingsPolicySnapshot {
    PlaybackAspectMode aspect{PlaybackAspectMode::Fit};
    PlaybackScaleMode scale{PlaybackScaleMode::Auto};
    PlaybackToneMappingMode tone_mapping{PlaybackToneMappingMode::Auto};
    PlaybackDeinterlaceMode deinterlace{PlaybackDeinterlaceMode::Auto};
    double playback_speed{1.0};
    double subtitle_size{1.0};
    bool valid{false};
};

struct PlaybackAspectModeRequest {
    PlaybackAspectMode mode{PlaybackAspectMode::Fit};
};

struct PlaybackScaleModeRequest {
    PlaybackScaleMode mode{PlaybackScaleMode::Auto};
};

struct PlaybackToneMappingModeRequest {
    PlaybackToneMappingMode mode{PlaybackToneMappingMode::Auto};
};

struct PlaybackDeinterlaceModeRequest {
    PlaybackDeinterlaceMode mode{PlaybackDeinterlaceMode::Auto};
};

struct PlaybackSpeedRequest {
    double speed{1.0};
};

struct SubtitleSizeRequest {
    double scale{1.0};
};

}  // namespace kivo::video::playback_graph