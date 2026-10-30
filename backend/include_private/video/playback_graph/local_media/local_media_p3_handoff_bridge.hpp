#pragma once

#include <cstdint>
#include <string>

#include "kivo/p3/playback_decision/decision_contracts.hpp"
#include "kivo/p3/playback_handoff/handoff_contracts.hpp"
#include "kivo/playback/control_plane/handoff/playback_open_envelope.hpp"
#include "video/video_plane/runtime/local_media/local_video_first_frame_pipeline.hpp"

namespace kivo::video::playback_graph::local_media {

struct LocalMediaP3HandoffStatus {
    bool candidate_set_created{false};
    bool handoff_created{false};
    bool p4_open_envelope_created{false};
    bool consumed_by_p7{false};
    char candidate_kind[32]{};
    char handoff_id[96]{};
    char candidate_set_id[96]{};
};

class LocalMediaP3HandoffBridge {
public:
    [[nodiscard]] bool build(
        std::uint64_t source_id,
        const kivo::video::video_plane::runtime::local_media::LocalMediaPipelineStatus&
            status) noexcept;
    void mark_consumed_by_p7() noexcept;
    void reset() noexcept;

    [[nodiscard]] const LocalMediaP3HandoffStatus& status() const noexcept;

private:
    void clear_strings() noexcept;

    LocalMediaP3HandoffStatus status_{};
    std::string source_ref_{};
    std::string selected_candidate_ref_{};
    kivo::p3::playback_decision::PlaybackCandidate selected_candidate_{};
    kivo::p3::playback_decision::PlaybackCandidateSet candidate_set_{};
    kivo::p3::playback_handoff::PlaybackHandoffContract handoff_{};
    kivo::playback::control_plane::handoff::P3SourceSessionRef p3_source_session_ref_{};
    kivo::playback::control_plane::handoff::P3CandidateSetRef p3_candidate_set_ref_{};
    kivo::playback::control_plane::handoff::P3SelectedCandidateRef p3_selected_candidate_ref_{};
    kivo::playback::control_plane::handoff::P3ProbeEvidenceView p3_probe_evidence_{};
    kivo::playback::control_plane::handoff::PlaybackOpenEnvelope p4_open_envelope_{};
};

}  // namespace kivo::video::playback_graph::local_media
