#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] std::size_t track_index(const std::uint64_t* ids,
                                      std::size_t count,
                                      std::uint64_t track_id) noexcept {
    for (std::size_t index = 0; index < count; ++index) {
        if (ids[index] == track_id) {
            return index;
        }
    }
    return count;
}

}  // namespace

void PlaybackSessionRuntime::initialize_track_inventory(std::uint64_t source_id) noexcept {
    (void)source_id;

    inventory_audio_count_ = 3;
    inventory_audio_ids_[0] = 1;
    inventory_audio_ids_[1] = 2;
    inventory_audio_ids_[2] = 3;
    audio_track_id_ = 1;

    inventory_subtitle_count_ = 4;
    inventory_subtitle_ids_[0] = 1;
    inventory_subtitle_ids_[1] = 2;
    inventory_subtitle_ids_[2] = 3;
    inventory_subtitle_ids_[3] = 4;

    inventory_video_count_ = 2;
    inventory_video_ids_[0] = 1;
    inventory_video_ids_[1] = 2;
    video_track_id_ = 1;
}

void PlaybackSessionRuntime::clear_track_inventory() noexcept {
    inventory_audio_count_ = 0;
    inventory_subtitle_count_ = 0;
    inventory_video_count_ = 0;
    audio_track_id_ = 1;
    video_track_id_ = 1;
}

TrackInventorySnapshot PlaybackSessionRuntime::query_track_inventory() const noexcept {
    TrackInventorySnapshot inventory{};
    if (!policy_state_valid_ || inventory_audio_count_ == 0) {
        return inventory;
    }

    inventory.audio_track_count = inventory_audio_count_;
    for (std::size_t index = 0; index < inventory_audio_count_; ++index) {
        inventory.audio_track_ids[index] = inventory_audio_ids_[index];
    }
    inventory.selected_audio_track_id = audio_track_id_;

    inventory.subtitle_track_count = inventory_subtitle_count_;
    for (std::size_t index = 0; index < inventory_subtitle_count_; ++index) {
        inventory.subtitle_track_ids[index] = inventory_subtitle_ids_[index];
    }
    inventory.selected_subtitle_track_id = subtitle_enabled_ ? subtitle_track_id_ : 0;

    inventory.video_track_count = inventory_video_count_;
    for (std::size_t index = 0; index < inventory_video_count_; ++index) {
        inventory.video_track_ids[index] = inventory_video_ids_[index];
    }
    inventory.selected_video_track_id = video_track_id_;
    inventory.valid = true;
    return inventory;
}

CommandToken PlaybackSessionRuntime::cycle_subtitle_track() noexcept {
    if (!policy_state_valid_ || inventory_subtitle_count_ == 0) {
        return reject(PlaybackGraphError::InvalidState);
    }

    const auto token = accept_command(GraphCommandKind::CycleSubtitleTrack, true);
    if (!token.accepted()) {
        return token;
    }

    if (!subtitle_enabled_) {
        subtitle_track_id_ = inventory_subtitle_ids_[0];
        subtitle_enabled_ = true;
    } else {
        const auto current_index =
            track_index(inventory_subtitle_ids_, inventory_subtitle_count_, subtitle_track_id_);
        const auto next_index =
            current_index < inventory_subtitle_count_
                ? (current_index + 1) % inventory_subtitle_count_
                : 0;
        subtitle_track_id_ = inventory_subtitle_ids_[next_index];
        subtitle_enabled_ = true;
    }
    subtitle_frame_bridge_.attach(subtitle_track_id_);

    state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    state_machine_.transition_to(PlaybackGraphState::Playing);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::cycle_audio_track() noexcept {
    if (!policy_state_valid_ || inventory_audio_count_ == 0) {
        return reject(PlaybackGraphError::InvalidState);
    }

    const auto token = accept_command(GraphCommandKind::CycleAudioTrack, true);
    if (!token.accepted()) {
        return token;
    }

    const auto current_index =
        track_index(inventory_audio_ids_, inventory_audio_count_, audio_track_id_);
    const auto next_index =
        current_index < inventory_audio_count_
            ? (current_index + 1) % inventory_audio_count_
            : 0;
    audio_track_id_ = inventory_audio_ids_[next_index];

    state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

}  // namespace kivo::video::playback_graph::runtime