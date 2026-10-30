#pragma once

#include "kivo/playback/control_plane/track/track_switch_fence.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"
#include "kivo/playback/control_plane/handoff/p3_probe_evidence_view.hpp"
#include "kivo/playback/control_plane/handoff/stream_layout_hint.hpp"
#include "kivo/playback/control_plane/handoff/interleave_profile.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::recovery {

/// AuthorizedFallback is an authorized fallback candidate for recovery.
struct AuthorizedFallback
{
    primitives::U64 fallback_id{0};
    track::FallbackAuthorizationKind authorization_kind{track::FallbackAuthorizationKind::ProviderAuthorized};
    handoff::P3SelectedCandidateRef candidate{};
    handoff::P3ReaderFactoryRef reader_factory{};
    const handoff::P3ProbeEvidenceView* probe_evidence{nullptr};
    const handoff::StreamLayoutHint* stream_layout_hint{nullptr};
    const handoff::InterleaveProfile* interleave_profile{nullptr};
};

} // namespace kivo::playback::control_plane::recovery
