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

CommandToken PlaybackSession::close() noexcept {
    auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? allocation_failure_token() : runtime->close();
}

PlaybackSessionSnapshot PlaybackSession::snapshot() const noexcept {
    const auto* runtime = as_runtime(impl_);
    return runtime == nullptr ? PlaybackSessionSnapshot{} : runtime->snapshot();
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
