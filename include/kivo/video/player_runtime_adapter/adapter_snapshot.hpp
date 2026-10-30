#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_session_snapshot.hpp"
#include "kivo/video/player_runtime_adapter/adapter_connection_status.hpp"
#include "kivo/video/player_runtime_adapter/adapter_types.hpp"

namespace kivo::video::player_runtime_adapter {

struct AdapterConnectionSnapshot {
    AdapterConnectionStatus playback_session{AdapterConnectionStatus::ConnectedToP7};
    AdapterConnectionStatus video_surface{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus timeline{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus buffered_ranges{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus video_telemetry{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus track_inventory{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus video_track_switch{AdapterConnectionStatus::ConnectedToP7};
    AdapterConnectionStatus audio_track_switch{AdapterConnectionStatus::ConnectedToP7};
    AdapterConnectionStatus subtitle_frame{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus subtitle_tracks{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus audio_volume{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus audio_device{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus fullscreen_screen_policy{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus playlist{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus user_settings_policy{AdapterConnectionStatus::NotConnectedToP7};
    AdapterConnectionStatus recovery_actions{AdapterConnectionStatus::NotConnectedToP7};
};

struct AdapterPlaylistItem {
    std::uint64_t index{0};
    std::string label{};
    bool current{false};
};

struct AdapterSnapshot {
    playback_graph::PlaybackSessionSnapshot p7_snapshot{};
    AdapterConnectionSnapshot connections{};

    RuntimePlaybackState playback_state{RuntimePlaybackState::NotCreated};
    playback_graph::PlaybackGraphError last_error{playback_graph::PlaybackGraphError::None};
    bool is_opening{false};
    bool is_playing{false};
    bool is_paused{false};
    bool is_buffering{false};
    bool closed{false};

    BufferReason buffer_reason{BufferReason::None};
    std::uint32_t rebuffer_count{0};

    std::int64_t position_ms{0};
    std::int64_t duration_ms{0};
    std::vector<AdapterTimeRange> buffered_ranges{};

    bool video_ready{false};
    AdapterVideoSize video_size{};
    double aspect_ratio{0.0};
    const char* hdr_mode{""};
    std::uint32_t dropped_frames{0};
    RenderState render_state{RenderState::NoSurface};

    double volume{1.0};
    bool muted{false};
    std::int64_t audio_delay_ms{0};
    bool fullscreen_like{false};
    std::int32_t screen_index{-1};
    bool screen_index_valid{false};
    bool auto_play_next{false};
    bool playlist_queue_connected{false};
    bool playlist_has_previous{false};
    bool playlist_has_next{false};
    bool playlist_has_next_episode{false};
    std::uint64_t playlist_current_index{0};
    std::uint64_t playlist_item_count{0};
    std::vector<AdapterPlaylistItem> playlist_items{};

    std::uint64_t selected_audio_track{0};
    std::vector<std::uint64_t> audio_track_ids{};
    std::vector<std::uint64_t> video_track_ids{};
    std::uint64_t selected_video_track{0};
    std::uint64_t selected_subtitle_track{0};
    std::vector<std::uint64_t> subtitle_track_ids{};
    bool subtitle_enabled{false};
    bool subtitle_frame_available{false};
    char subtitle_primary[256]{};
    char subtitle_secondary[256]{};
    std::int64_t subtitle_delay_ms{0};

    AdapterAspectMode aspect_mode{AdapterAspectMode::Fit};
    AdapterScaleMode scale_mode{AdapterScaleMode::Auto};
    AdapterToneMappingMode tone_mapping_mode{AdapterToneMappingMode::Auto};
    AdapterDeinterlaceMode deinterlace_mode{AdapterDeinterlaceMode::Auto};
    double playback_speed{1.0};
    double subtitle_size{1.0};
};

}  // namespace kivo::video::player_runtime_adapter
