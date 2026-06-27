#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/output_boundary/i_media_stream_pipeline.hpp"
#include "kivo/playback/control_plane/timeline/playback_timeline_state.hpp"
#include "kivo/playback/control_plane/rate/playback_rate_ratio.hpp"

namespace kivo::playback::control_plane::output_boundary {

/// ClockHandoverPolicy defines how P4 hands clock to P6.
enum class ClockHandoverPolicy
{
    P4OwnsTimeline,
    P6MayPromoteAudioClock,
    ExternalClock
};

/// AudioPipelineInputPlan is the P4 -> P6 audio output boundary plan.
/// P4 must NOT output audio device or decoded audio samples.
struct AudioPipelineInputPlan
{
    revision::SessionId session_id{};
    revision::StreamRevision stream_revision{};
    IMediaStreamPipeline* encoded_stream{nullptr};
    const timeline::PlaybackTimelineState* timeline_snapshot{nullptr};
    rate::PlaybackRateRatio playback_rate{};
    ClockHandoverPolicy clock_handover_policy{ClockHandoverPolicy::P4OwnsTimeline};
};

} // namespace kivo::playback::control_plane::output_boundary
