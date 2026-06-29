#include <type_traits>

#include "kivo/video/playback_graph/all_public_headers.hpp"

using namespace kivo::video::playback_graph;

static_assert(kP7ApiVersion.major == 1);
static_assert(kP7ApiVersion.minor == 9);
static_assert(kP7ApiVersion.patch == 1);

static_assert(std::is_trivially_copyable_v<PlaybackCommandId>);
static_assert(std::is_trivially_copyable_v<GraphCorrelationId>);
static_assert(std::is_trivially_copyable_v<PlaybackRevision>);
static_assert(std::is_trivially_copyable_v<GraphSnapshotRevision>);
static_assert(std::is_trivially_copyable_v<GenerationStamp>);

static_assert(std::is_class_v<PlaybackSession>);
static_assert(!std::is_copy_constructible_v<PlaybackSession>);
static_assert(!std::is_copy_assignable_v<PlaybackSession>);
static_assert(std::is_move_constructible_v<PlaybackSession>);
static_assert(std::is_move_assignable_v<PlaybackSession>);

using OpenCommand = CommandToken (PlaybackSession::*)(const OpenRequest&) noexcept;
using EmptyCommand = CommandToken (PlaybackSession::*)() noexcept;
using SeekCommand = CommandToken (PlaybackSession::*)(const SeekRequest&) noexcept;
using AudioTrackSwitchCommand =
    CommandToken (PlaybackSession::*)(const AudioTrackSwitchRequest&) noexcept;
using VideoTrackSwitchCommand =
    CommandToken (PlaybackSession::*)(const VideoTrackSwitchRequest&) noexcept;
using AvTrackSetSwitchCommand =
    CommandToken (PlaybackSession::*)(const AvTrackSetSwitchRequest&) noexcept;
using SnapshotQuery = PlaybackSessionSnapshot (PlaybackSession::*)() const noexcept;
using CommandQuery =
    CommandLifecycleSnapshot (PlaybackSession::*)(PlaybackCommandId) const noexcept;
using SubscribeCommand = SubscriptionToken (PlaybackSession::*)(GraphObserverHandle) noexcept;
using UnsubscribeCommand = void (PlaybackSession::*)(SubscriptionToken) noexcept;
using SystemEventCommand = CommandToken (PlaybackSession::*)(const SystemEvent&) noexcept;

static_assert(std::is_same_v<decltype(&PlaybackSession::open), OpenCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::start), EmptyCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::pause), EmptyCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::resume), EmptyCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::seek), SeekCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::switch_audio_track),
                             AudioTrackSwitchCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::switch_video_track),
                             VideoTrackSwitchCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::switch_av_track_set),
                             AvTrackSetSwitchCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::close), EmptyCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::snapshot), SnapshotQuery>);
static_assert(std::is_same_v<decltype(&PlaybackSession::query_command), CommandQuery>);
static_assert(std::is_same_v<decltype(&PlaybackSession::subscribe_events), SubscribeCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::unsubscribe), UnsubscribeCommand>);
static_assert(std::is_same_v<decltype(&PlaybackSession::inject_system_event),
                             SystemEventCommand>);

int main() {
    CommandToken token{};
    if (token.accepted()) {
        return 1;
    }

    token.command_id.value = 1;
    if (!token.accepted()) {
        return 2;
    }

    CommandLifecycleSnapshot lifecycle{};
    if (lifecycle.state != CommandLifecycleState::NotFound) {
        return 3;
    }

    PlaybackSessionSnapshot snapshot{};
    if (snapshot.state != PlaybackGraphState::NotCreated) {
        return 4;
    }

    PlaybackGraphPolicy policy{};
    if (policy.tuning.seqlock_max_attempt_count != 3) {
        return 5;
    }

    SeekRequest seek{};
    seek.target_timeline_ms = 42;
    if (seek.kind != SeekKind::Absolute || seek.target_timeline_ms != 42) {
        return 6;
    }

    AvTrackSetSwitchRequest switch_request{};
    if (switch_request.atomicity != TrackSwitchAtomicity::AtomicAllOrNothing) {
        return 7;
    }

    PlaybackGraphEvent event{};
    event.kind = PlaybackGraphEventKind::CommandAccepted;
    if (event.kind != PlaybackGraphEventKind::CommandAccepted) {
        return 8;
    }

    SnapshotQueryResult<AudioQueueSnapshot> audio_query{};
    if (!audio_query.ok()) {
        return 9;
    }

    return 0;
}
