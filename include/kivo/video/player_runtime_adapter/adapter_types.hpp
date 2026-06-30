#pragma once

#include <cstdint>

namespace kivo::video::player_runtime_adapter {

enum class RuntimePlaybackState {
    NotCreated,
    Created,
    Opening,
    Ready,
    Starting,
    Buffering,
    Playing,
    Rebuffering,
    Stalled,
    Pausing,
    Paused,
    Resuming,
    Seeking,
    TrackSwitching,
    EosReached,
    Recovering,
    Failed,
    Closing,
    Closed
};

enum class BufferReason {
    None,
    Opening,
    InitialBuffer,
    Rebuffer,
    Stalled,
    Seeking
};

enum class RenderState {
    NoSurface,
    SurfaceReady,
    SurfaceReleased
};

enum class AdapterShortcutAction {
    TogglePlayPause,
    SeekBackward,
    SeekForward,
    VolumeUp,
    VolumeDown,
    ToggleFullscreen,
    ExitFullscreen,
    ToggleMute,
    Subtitle,
    AudioTrack
};

enum class AdapterAspectMode {
    Fit,
    Fill,
    Stretch,
    Original
};

enum class AdapterToneMappingMode {
    Auto,
    Off,
    Sdr,
    HdrPassthrough
};

struct AdapterOpenRequest {
    std::uint64_t source_id{0};
    std::uint64_t initial_audio_track_id{0};
    std::uint64_t initial_video_track_id{0};
    bool audio_selected{true};
    bool video_selected{true};
};

struct AdapterSurfaceHandle {
    std::uintptr_t native_handle{0};
};

struct AdapterTimeRange {
    std::int64_t start_ms{0};
    std::int64_t end_ms{0};
};

struct AdapterVideoSize {
    std::int32_t width{0};
    std::int32_t height{0};
};

}  // namespace kivo::video::player_runtime_adapter
