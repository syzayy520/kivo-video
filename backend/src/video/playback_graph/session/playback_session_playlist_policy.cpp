#include "video/playback_graph/session/playback_session_runtime.hpp"

#include <algorithm>
#include <cstring>

#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/source_runtime/local_media/local_media_directory_queue.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] bool navigation_available(const PlaylistPolicySnapshot& policy,
                                        const bool requested_item) noexcept {
    return policy.queue_connected && requested_item;
}

void copy_label(char* const target, const std::size_t target_size, const std::string& label) {
    if (target == nullptr || target_size == 0) {
        return;
    }
    const std::size_t copy_size = std::min(target_size - 1, label.size());
    std::memcpy(target, label.data(), copy_size);
    target[copy_size] = '\0';
}

}  // namespace

void PlaybackSessionRuntime::rebuild_playlist_queue_for_local_media(
    const std::string& path) noexcept {
    const bool auto_play_next = playlist_policy_.auto_play_next;
    playlist_policy_ = {};
    playlist_policy_.valid = true;
    playlist_policy_.auto_play_next = auto_play_next;
    playlist_item_paths_.clear();

    const auto queue =
        source_runtime::local_media::build_local_media_directory_queue(path);
    if (!queue.valid || queue.items.empty()) {
        return;
    }

    playlist_policy_.queue_connected = true;
    playlist_policy_.current_index = queue.current_index;
    playlist_policy_.item_count = static_cast<std::uint64_t>(queue.items.size());
    playlist_policy_.visible_item_count =
        std::min(queue.items.size(), PlaylistPolicySnapshot::kMaxItems);
    playlist_policy_.has_previous = queue.current_index > 0;
    playlist_policy_.has_next = queue.current_index + 1 < queue.items.size();
    playlist_policy_.has_next_episode = playlist_policy_.has_next;

    playlist_item_paths_.reserve(queue.items.size());
    for (std::size_t i = 0; i < queue.items.size(); ++i) {
        playlist_item_paths_.push_back(queue.items[i].path);
        if (i >= PlaylistPolicySnapshot::kMaxItems) {
            continue;
        }
        auto& item = playlist_policy_.items[i];
        item.index = queue.items[i].index;
        item.current = queue.items[i].current;
        copy_label(item.label, PlaylistItemSnapshot::kMaxLabelBytes, queue.items[i].label);
    }
}

void PlaybackSessionRuntime::clear_playlist_queue() noexcept {
    playlist_policy_ = {};
    playlist_item_paths_.clear();
}

CommandToken PlaybackSessionRuntime::navigate_to_playlist_index(
    const std::uint64_t index) noexcept {
    playlist_policy_.valid = true;
    if (!playlist_policy_.queue_connected || index >= playlist_item_paths_.size()) {
        return reject(PlaybackGraphError::PolicyUnsupportedByCapability);
    }

    const auto previous_state = state_machine_.state();
    const bool should_start_after_open = previous_state == PlaybackGraphState::Playing ||
                                         previous_state == PlaybackGraphState::Starting ||
                                         previous_state == PlaybackGraphState::Buffering ||
                                         previous_state == PlaybackGraphState::Rebuffering;
    const auto token = accept_command(GraphCommandKind::PlaylistNavigate, false);
    if (!token.accepted()) {
        return token;
    }

    const std::string path = playlist_item_paths_[static_cast<std::size_t>(index)];
    local_media_continuous_.close();
    local_media_pipeline_.close();
    local_media_p3_handoff_.reset();
    clear_track_inventory();
    subtitle_track_id_ = 0;
    subtitle_enabled_ = false;
    subtitle_frame_bridge_.reset();
    state_machine_.transition_to(PlaybackGraphState::TrackSwitching);

    OpenRequest request{};
    request.source_id =
        local_media::LocalMediaSourceRegistry::instance().register_path(path);
    if (!try_open_local_media(request)) {
        complete_if_accepted(
            token, CommandTerminalStatus::Failed, PlaybackGraphError::SourceOpenFailed);
        publish_current_snapshot();
        return token;
    }

    state_machine_.transition_to(PlaybackGraphState::Ready);
    if (should_start_after_open) {
        state_machine_.transition_to(PlaybackGraphState::Starting);
        if (!try_start_local_media()) {
            complete_if_accepted(
                token, CommandTerminalStatus::Failed, PlaybackGraphError::InvalidState);
            publish_current_snapshot();
            return token;
        }
        state_machine_.transition_to(PlaybackGraphState::Playing);
    }

    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

CommandToken PlaybackSessionRuntime::next() noexcept {
    playlist_policy_.valid = true;
    if (!navigation_available(playlist_policy_, playlist_policy_.has_next)) {
        return reject(PlaybackGraphError::PolicyUnsupportedByCapability);
    }
    return navigate_to_playlist_index(playlist_policy_.current_index + 1);
}

CommandToken PlaybackSessionRuntime::previous() noexcept {
    playlist_policy_.valid = true;
    if (!navigation_available(playlist_policy_, playlist_policy_.has_previous)) {
        return reject(PlaybackGraphError::PolicyUnsupportedByCapability);
    }
    return navigate_to_playlist_index(playlist_policy_.current_index - 1);
}

CommandToken PlaybackSessionRuntime::play_next_episode() noexcept {
    playlist_policy_.valid = true;
    if (!navigation_available(playlist_policy_, playlist_policy_.has_next_episode)) {
        return reject(PlaybackGraphError::PolicyUnsupportedByCapability);
    }
    return navigate_to_playlist_index(playlist_policy_.current_index + 1);
}

CommandToken PlaybackSessionRuntime::play_playlist_item(
    const PlaylistItemSelectRequest& request) noexcept {
    return navigate_to_playlist_index(request.index);
}

CommandToken PlaybackSessionRuntime::set_auto_play_next(
    const AutoPlayNextRequest& request) noexcept {
    const auto token = accept_command(GraphCommandKind::SetPlaylistPolicy, false);
    if (!token.accepted()) {
        return token;
    }
    playlist_policy_.valid = true;
    playlist_policy_.auto_play_next = request.enabled;
    complete_if_accepted(token, CommandTerminalStatus::Completed, PlaybackGraphError::None);
    publish_current_snapshot();
    return token;
}

PlaylistPolicySnapshot PlaybackSessionRuntime::query_playlist_policy() const noexcept {
    return playlist_policy_;
}

}  // namespace kivo::video::playback_graph::runtime
