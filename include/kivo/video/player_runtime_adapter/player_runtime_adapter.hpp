#pragma once

#include <cstdint>
#include <string_view>

#include "kivo/video/playback_graph/local_media_playback_query.hpp"
#include "kivo/video/playback_graph/playback_session.hpp"
#include "kivo/video/player_runtime_adapter/adapter_command_result.hpp"
#include "kivo/video/player_runtime_adapter/adapter_snapshot.hpp"
#include "kivo/video/player_runtime_adapter/adapter_types.hpp"

namespace kivo::video::player_runtime_adapter {

class PlayerRuntimeAdapter {
public:
    PlayerRuntimeAdapter();
    explicit PlayerRuntimeAdapter(const playback_graph::PlaybackGraphPolicy& policy);
    ~PlayerRuntimeAdapter();

    PlayerRuntimeAdapter(const PlayerRuntimeAdapter&) = delete;
    PlayerRuntimeAdapter& operator=(const PlayerRuntimeAdapter&) = delete;
    PlayerRuntimeAdapter(PlayerRuntimeAdapter&&) noexcept;
    PlayerRuntimeAdapter& operator=(PlayerRuntimeAdapter&&) noexcept;

    [[nodiscard]] AdapterCommandResult open(const AdapterOpenRequest& request) noexcept;
    [[nodiscard]] AdapterCommandResult open_media_id(std::uint64_t media_id) noexcept;
    [[nodiscard]] AdapterCommandResult open_local_media_file(std::string_view path) noexcept;
    [[nodiscard]] AdapterCommandResult close() noexcept;
    [[nodiscard]] AdapterCommandResult release_surface() noexcept;

    [[nodiscard]] AdapterCommandResult attach_video_surface(AdapterSurfaceHandle handle) noexcept;

    [[nodiscard]] AdapterCommandResult play() noexcept;
    [[nodiscard]] AdapterCommandResult pause() noexcept;
    [[nodiscard]] AdapterCommandResult toggle_play_pause() noexcept;
    [[nodiscard]] AdapterCommandResult seek(std::int64_t position_ms) noexcept;
    [[nodiscard]] AdapterCommandResult seek_relative(std::int64_t delta_ms) noexcept;
    [[nodiscard]] AdapterCommandResult step_forward(std::int64_t step_ms) noexcept;
    [[nodiscard]] AdapterCommandResult step_backward(std::int64_t step_ms) noexcept;
    [[nodiscard]] AdapterCommandResult stop() noexcept;

    [[nodiscard]] AdapterCommandResult select_video_track(std::uint64_t track_id) noexcept;
    [[nodiscard]] AdapterCommandResult select_audio_track(std::uint64_t track_id) noexcept;
    [[nodiscard]] AdapterCommandResult select_subtitle_track(std::uint64_t track_id) noexcept;
    [[nodiscard]] AdapterCommandResult disable_subtitle() noexcept;

    [[nodiscard]] AdapterCommandResult set_volume(double value) noexcept;
    [[nodiscard]] AdapterCommandResult set_muted(bool muted) noexcept;
    [[nodiscard]] AdapterCommandResult select_audio_device(std::string_view device_id) noexcept;

    [[nodiscard]] AdapterCommandResult enter_fullscreen() noexcept;
    [[nodiscard]] AdapterCommandResult exit_fullscreen() noexcept;
    [[nodiscard]] AdapterCommandResult move_to_screen(std::int32_t screen_index) noexcept;

    [[nodiscard]] AdapterCommandResult next() noexcept;
    [[nodiscard]] AdapterCommandResult previous() noexcept;
    [[nodiscard]] AdapterCommandResult play_next_episode() noexcept;
    [[nodiscard]] AdapterCommandResult set_auto_play_next(bool enabled) noexcept;

    [[nodiscard]] AdapterCommandResult handle_shortcut(AdapterShortcutAction action) noexcept;
    [[nodiscard]] AdapterCommandResult handle_shortcut_name(std::string_view name) noexcept;

    [[nodiscard]] AdapterCommandResult set_subtitle_size(double scale) noexcept;
    [[nodiscard]] AdapterCommandResult set_subtitle_delay(std::int64_t delay_ms) noexcept;
    [[nodiscard]] AdapterCommandResult set_audio_delay(std::int64_t delay_ms) noexcept;
    [[nodiscard]] AdapterCommandResult set_aspect_mode(AdapterAspectMode mode) noexcept;
    [[nodiscard]] AdapterCommandResult set_scale_mode(AdapterScaleMode mode) noexcept;
    [[nodiscard]] AdapterCommandResult set_tone_mapping_mode(AdapterToneMappingMode mode) noexcept;
    [[nodiscard]] AdapterCommandResult set_deinterlace_mode(AdapterDeinterlaceMode mode) noexcept;
    [[nodiscard]] AdapterCommandResult set_playback_speed(double speed) noexcept;

    [[nodiscard]] AdapterCommandResult retry() noexcept;
    [[nodiscard]] AdapterCommandResult reopen() noexcept;
    [[nodiscard]] AdapterCommandResult copy_diagnostics() noexcept;

    [[nodiscard]] AdapterSnapshot snapshot() const noexcept;
    [[nodiscard]] playback_graph::LocalMediaPlaybackQuery query_local_media_playback() const noexcept;

private:
    playback_graph::PlaybackSession session_;
    std::uint32_t rebuffer_count_{0};
};

}  // namespace kivo::video::player_runtime_adapter
