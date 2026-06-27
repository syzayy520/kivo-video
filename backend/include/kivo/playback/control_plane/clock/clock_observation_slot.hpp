#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::clock {

/// ClockSource enumerates the source of a clock observation.
enum class ClockSource
{
    AudioOutput,
    Decoder,
    External
};

/// ClockObservation is a single clock observation sample.
struct ClockObservation
{
    revision::SessionId session_id{};
    revision::SessionGeneration generation{};
    revision::StreamRevision stream_revision{};
    revision::StreamRevision based_on_revision{};
    ClockSource source{ClockSource::AudioOutput};
    primitives::MediaTimeMs observed_position_ms{};
    primitives::MonotonicTimestamp observed_at{};
    primitives::Milliseconds playback_rate{1000};
    std::optional<primitives::Milliseconds> drift_ms{};
};

/// ClockObservationSlot provides realtime-safe publish for AudioOutput.
/// Implementation contract for ClockSource::AudioOutput:
/// - Must use publish_latest_realtime_safe.
/// - Wait-free or bounded lock-free.
/// - No mutex. No heap allocation. No waiting for Control Thread.
/// - No EventBus publish. No callback.
/// - Recommended: sequence counter / double-buffer atomic slot.
/// Stale rules:
/// - generation mismatch: drop.
/// - stream_revision mismatch: drop or record stale observation.
/// - based_on_revision older than current: drop or record stale.
/// - based_on_revision newer than current: reject invalid.
class ClockObservationSlot
{
public:
    virtual ~ClockObservationSlot() = default;

    virtual void publish_latest_realtime_safe(const ClockObservation& observation) = 0;
    virtual void publish_latest_non_realtime(const ClockObservation& observation) = 0;
    virtual std::optional<ClockObservation> load_latest(ClockSource source) const = 0;
    virtual std::optional<ClockObservation> snapshot() const = 0;
};

} // namespace kivo::playback::control_plane::clock
