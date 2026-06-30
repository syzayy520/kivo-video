#pragma once

namespace kivo::video::player_runtime_adapter {

enum class AdapterConnectionStatus {
    ConnectedToP7,
    NotConnectedToP7
};

enum class AdapterCommandStatus {
    Accepted,
    RejectedByP7,
    NotConnectedToP7
};

enum class AdapterMissingP7Api {
    None,
    VideoSurface,
    PositionTimeline,
    BufferedRanges,
    TrackInventory,
    SubtitleFrame,
    SubtitleTrackSwitch,
    AudioVolume,
    AudioDevice,
    FullscreenWindowScreen,
    Playlist,
    UserSettingsPolicy,
    RecoveryAction,
    DiagnosticsClipboard,
    StopWithoutClose
};

}  // namespace kivo::video::player_runtime_adapter
