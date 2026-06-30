#include "kivo/video/playback_graph/playback_session.hpp"

#include <new>
#include <utility>

#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph {
namespace {

using Runtime = runtime::PlaybackSessionRuntime;

[[nodiscard]] Runtime* as_runtime(void* impl) noexcept {
    return static_cast<Runtime*>(impl);
}

[[nodiscard]] const Runtime* as_runtime(const void* impl) noexcept {
    return static_cast<const Runtime*>(impl);
}

[[nodiscard]] CommandToken allocation_failure_token() noexcept {
    CommandToken token{};
    token.immediate_error = PlaybackGraphError::OutOfMemory;
    return token;
}

}  // namespace

PlaybackSession::PlaybackSession() : PlaybackSession(PlaybackGraphPolicy{}) {}

PlaybackSession::PlaybackSession(const PlaybackGraphPolicy& policy)
    : impl_(new (std::nothrow) Runtime(policy)) {}

PlaybackSession::~PlaybackSession() {
    delete as_runtime(impl_);
}

PlaybackSession::PlaybackSession(PlaybackSession&& other) noexcept : impl_(other.impl_) {
    other.impl_ = nullptr;
}

PlaybackSession& PlaybackSession::operator=(PlaybackSession&& other) noexcept {
    if (this != &other) {
        delete as_runtime(impl_);
        impl_ = std::exchange(other.impl_, nullptr);
    }
    return *this;
}

CommandToken PlaybackSession::open(const OpenRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->open(request);
}

CommandToken PlaybackSession::start() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->start();
}

CommandToken PlaybackSession::pause() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->pause();
}

CommandToken PlaybackSession::resume() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->resume();
}

CommandToken PlaybackSession::seek(const SeekRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->seek(request);
}

CommandToken PlaybackSession::switch_audio_track(const AudioTrackSwitchRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->switch_audio_track(request);
}

CommandToken PlaybackSession::switch_video_track(const VideoTrackSwitchRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->switch_video_track(request);
}

CommandToken PlaybackSession::switch_av_track_set(const AvTrackSetSwitchRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->switch_av_track_set(request);
}

CommandToken PlaybackSession::switch_subtitle_track(
    const SubtitleTrackSwitchRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token()
                              : runtime->switch_subtitle_track(request);
}

CommandToken PlaybackSession::disable_subtitle() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->disable_subtitle();
}

CommandToken PlaybackSession::stop() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->stop();
}

CommandToken PlaybackSession::set_subtitle_delay(const SubtitleDelayRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_subtitle_delay(request);
}

CommandToken PlaybackSession::set_audio_volume(const AudioVolumeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_audio_volume(request);
}

CommandToken PlaybackSession::set_audio_muted(const AudioMuteRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_audio_muted(request);
}

CommandToken PlaybackSession::select_audio_output_device(
    const AudioDeviceSelectRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token()
                              : runtime->select_audio_output_device(request);
}

CommandToken PlaybackSession::set_audio_delay(const AudioDelayRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_audio_delay(request);
}

CommandToken PlaybackSession::set_aspect_mode(const PlaybackAspectModeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_aspect_mode(request);
}

CommandToken PlaybackSession::set_scale_mode(const PlaybackScaleModeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_scale_mode(request);
}

CommandToken PlaybackSession::set_tone_mapping_mode(
    const PlaybackToneMappingModeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token()
                              : runtime->set_tone_mapping_mode(request);
}

CommandToken PlaybackSession::set_deinterlace_mode(
    const PlaybackDeinterlaceModeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token()
                              : runtime->set_deinterlace_mode(request);
}

CommandToken PlaybackSession::set_playback_speed(const PlaybackSpeedRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_playback_speed(request);
}

CommandToken PlaybackSession::set_subtitle_size(const SubtitleSizeRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->set_subtitle_size(request);
}

CommandToken PlaybackSession::close() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->close();
}

CommandToken PlaybackSession::attach_video_surface(
    const VideoSurfaceBindingRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->attach_video_surface(request);
}

CommandToken PlaybackSession::detach_video_surface() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->detach_video_surface();
}

CommandToken PlaybackSession::release_video_surface() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->release_video_surface();
}

CommandToken PlaybackSession::retry(const RecoveryActionRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->retry(request);
}

CommandToken PlaybackSession::reopen(const RecoveryActionRequest& request) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->reopen(request);
}

PlaybackSessionSnapshot PlaybackSession::snapshot() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? PlaybackSessionSnapshot{} : runtime->snapshot();
}

PlaybackTimelineSnapshot PlaybackSession::query_timeline() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? PlaybackTimelineSnapshot{} : runtime->query_timeline();
}

VideoSurfaceSnapshot PlaybackSession::query_video_surface() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? VideoSurfaceSnapshot{} : runtime->query_video_surface();
}

DiagnosticsSummary PlaybackSession::query_diagnostics_summary() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? DiagnosticsSummary{} : runtime->query_diagnostics_summary();
}

SnapshotQueryResult<ClockSnapshot> PlaybackSession::query_clock() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? SnapshotQueryResult<ClockSnapshot>{} : runtime->query_clock();
}

SnapshotQueryResult<AudioQueueSnapshot> PlaybackSession::query_audio_queue() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? SnapshotQueryResult<AudioQueueSnapshot>{}
                              : runtime->query_audio_queue();
}

SnapshotQueryResult<VideoQueueSnapshot> PlaybackSession::query_video_queue() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? SnapshotQueryResult<VideoQueueSnapshot>{}
                              : runtime->query_video_queue();
}

SubtitleSnapshot PlaybackSession::query_subtitle() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? SubtitleSnapshot{} : runtime->query_subtitle();
}

AudioOutputPolicySnapshot PlaybackSession::query_audio_output_policy() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? AudioOutputPolicySnapshot{} : runtime->query_audio_output_policy();
}

PlaybackSettingsPolicySnapshot PlaybackSession::query_playback_settings_policy() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? PlaybackSettingsPolicySnapshot{}
                            : runtime->query_playback_settings_policy();
}

CommandLifecycleSnapshot PlaybackSession::query_command(PlaybackCommandId id) const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? CommandLifecycleSnapshot{} : runtime->query_command(id);
}

SubscriptionToken PlaybackSession::subscribe_events(GraphObserverHandle observer) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? SubscriptionToken{} : runtime->subscribe_events(observer);
}

void PlaybackSession::unsubscribe(SubscriptionToken token) noexcept {
    auto* runtime = as_runtime(impl_);
    if (runtime != nullptr) {
        runtime->unsubscribe(token);
    }
}

CommandToken PlaybackSession::inject_system_event(const SystemEvent& event) noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->inject_system_event(event);
}

}  // namespace kivo::video::playback_graph
