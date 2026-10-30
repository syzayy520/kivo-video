#include "video/playback_graph/session/playback_session_runtime.hpp"

#include "video/playback_graph/local_media/local_media_audio_display_filter.hpp"
#include "video/playback_graph/local_media/local_media_subtitle_display_filter.hpp"

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

[[nodiscard]] std::uint64_t track_id_from_stream_index(const std::int32_t stream_index) noexcept {
    return stream_index >= 0 ? static_cast<std::uint64_t>(stream_index) + 1u : 0u;
}

[[nodiscard]] std::int32_t stream_index_from_track_id(const std::uint64_t track_id) noexcept {
    return track_id > 0 ? static_cast<std::int32_t>(track_id - 1) : -1;
}

void copy_stream_tracks(const std::int32_t* stream_indices,
                        const std::size_t stream_count,
                        std::uint64_t* track_ids,
                        std::size_t& track_count) noexcept {
    track_count = 0;
    const std::size_t capped =
        stream_count < TrackInventorySnapshot::kMaxTracks ? stream_count
                                                          : TrackInventorySnapshot::kMaxTracks;
    for (std::size_t index = 0; index < capped; ++index) {
        const auto id = track_id_from_stream_index(stream_indices[index]);
        if (id != 0) {
            track_ids[track_count++] = id;
        }
    }
}

[[nodiscard]] std::string subtitle_codec_for_stream(
    const std::int32_t stream_index,
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus&
        status) {
    for (std::size_t index = 0; index < status.subtitle_stream_count; ++index) {
        if (status.subtitle_stream_indices[index] == stream_index) {
            return status.subtitle_stream_codecs[index];
        }
    }
    return {};
}

[[nodiscard]] PlaybackGraphState resume_state_after_switch(
    const PlaybackGraphState previous_state) noexcept {
    if (previous_state == PlaybackGraphState::Paused) {
        return PlaybackGraphState::Paused;
    }
    if (previous_state == PlaybackGraphState::Ready) {
        return PlaybackGraphState::Ready;
    }
    return PlaybackGraphState::Playing;
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

void PlaybackSessionRuntime::initialize_local_media_track_inventory(
    const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus&
        status) noexcept {
    clear_track_inventory();

    local_media::copy_displayable_audio_tracks(
        status, inventory_audio_ids_, inventory_audio_count_);
    local_media::copy_displayable_subtitle_tracks(
        status, inventory_subtitle_ids_, inventory_subtitle_count_);
    copy_stream_tracks(status.video_stream_indices,
                       status.video_stream_count,
                       inventory_video_ids_,
                       inventory_video_count_);

    audio_track_id_ = local_media::preferred_displayable_audio_track_id(status);
    video_track_id_ = inventory_video_count_ > 0 ? inventory_video_ids_[0] : 0;
    subtitle_track_id_ = 0;
    subtitle_enabled_ = false;
    subtitle_frame_bridge_.detach();
}

void PlaybackSessionRuntime::clear_track_inventory() noexcept {
    for (std::size_t index = 0; index < TrackInventorySnapshot::kMaxTracks; ++index) {
        inventory_audio_ids_[index] = 0;
        inventory_subtitle_ids_[index] = 0;
        inventory_video_ids_[index] = 0;
    }
    inventory_audio_count_ = 0;
    inventory_subtitle_count_ = 0;
    inventory_video_count_ = 0;
    audio_track_id_ = 0;
    video_track_id_ = 0;
}

TrackInventorySnapshot PlaybackSessionRuntime::query_track_inventory() const noexcept {
    TrackInventorySnapshot inventory{};
    if (!policy_state_valid_ ||
        (inventory_audio_count_ == 0 && inventory_subtitle_count_ == 0 &&
         inventory_video_count_ == 0)) {
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

    const auto previous_state = state_machine_.state();
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
    if (local_media_continuous_.is_active()) {
        const auto& status = local_media_pipeline_.status();
        const std::int32_t stream_index = stream_index_from_track_id(subtitle_track_id_);
        local_media_continuous_.select_subtitle_track(
            subtitle_track_id_,
            stream_index,
            subtitle_codec_for_stream(stream_index, status));
    } else {
        subtitle_frame_bridge_.attach(subtitle_track_id_, subtitle_fixture_source_allowed_);
    }

    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    state_machine_.transition_to(resume_state_after_switch(previous_state));
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

    const auto previous_state = state_machine_.state();
    const auto current_index =
        track_index(inventory_audio_ids_, inventory_audio_count_, audio_track_id_);
    const auto next_index =
        current_index < inventory_audio_count_
            ? (current_index + 1) % inventory_audio_count_
            : 0;
    audio_track_id_ = inventory_audio_ids_[next_index];

    if (previous_state != PlaybackGraphState::Ready) {
        state_machine_.transition_to(PlaybackGraphState::TrackSwitching);
    }
    state_machine_.transition_to(resume_state_after_switch(previous_state));
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

}  // namespace kivo::video::playback_graph::runtime
