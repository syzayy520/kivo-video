#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/output_boundary/i_media_stream_pipeline.hpp"
#include "kivo/playback/control_plane/handoff/stream_layout_hint.hpp"
#include "kivo/playback/control_plane/handoff/interleave_profile.hpp"
#include "kivo/playback/control_plane/handoff/p3_probe_evidence_view.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"

namespace kivo::playback::control_plane::output_boundary {

/// VideoPipelineInputPlan is the P4 -> P5 video output boundary plan.
/// P4 must NOT output decoded frame, render surface, audio device,
/// subtitle bitmap, UI event, DRM decrypted sample, segment scheduler,
/// platform media session, or AirPlay/Chromecast session.
struct VideoPipelineInputPlan
{
    revision::SessionId session_id{};
    revision::StreamRevision stream_revision{};
    handoff::P3SelectedCandidateRef selected_candidate{};
    IMediaStreamPipeline* encoded_stream{nullptr};
    const handoff::StreamLayoutHint* layout_hint{nullptr};
    const handoff::InterleaveProfile* interleave_profile{nullptr};
    const handoff::P3ProbeEvidenceView* probe_evidence{nullptr};
};

} // namespace kivo::playback::control_plane::output_boundary
