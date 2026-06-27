#pragma once

#include "kivo/playback/control_plane/contracts/stream_pull_result.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::output_boundary {

/// PullRequest is the request shape for IMediaStreamPipeline::pull.
struct PullRequest
{
    revision::RevisionStamp stamp{};
    primitives::Bytes max_bytes{};
    primitives::PullDeadline deadline{};
};

/// DownstreamFeedbackRequest wraps downstream demand/ack/failure feedback.
struct DownstreamFeedbackRequest
{
    primitives::U64 kind{0}; // DownstreamFeedbackKind value
    const void* payload{nullptr};
};

/// ClockObservation wraps a P6 clock observation for the realtime slot.
struct ClockObservation
{
    const void* observation{nullptr};
};

/// SubscriptionMask is a bitmask of event categories.
using SubscriptionMask = primitives::U64;

/// IMediaStreamPipeline is the single logical consumer pipeline interface.
/// Rules:
/// - IMediaStreamPipeline is single logical consumer.
/// - Recommended: only P5 demux worker calls pull().
/// - If multiple threads call pull(), Pipeline must serialize into one ordered stream.
/// - Multiple independent consumers directly pulling encoded bytes are forbidden.
/// - P4 does NOT demux pulled data.
class IMediaStreamPipeline
{
public:
    virtual ~IMediaStreamPipeline() = default;

    virtual contracts::StreamPullResult pull(const PullRequest& request) = 0;
    virtual void snapshot() = 0;
    virtual void submit_feedback(const DownstreamFeedbackRequest& request) = 0;
    virtual void submit_clock_observation(const ClockObservation& observation) = 0;
    virtual void subscribe(SubscriptionMask mask, void (*callback)(void*, SubscriptionMask), void* user_data) = 0;
};

} // namespace kivo::playback::control_plane::output_boundary
