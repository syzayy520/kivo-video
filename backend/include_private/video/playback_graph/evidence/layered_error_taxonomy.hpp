#pragma once

#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

[[nodiscard]] EvidenceSeverity severity_for_command_error(PlaybackGraphError error) noexcept;
[[nodiscard]] GraphEvidenceReason evidence_reason_for_late_discard() noexcept;
[[nodiscard]] bool architecture_failure_user_facing(ArchitectureGuardFailure failure) noexcept;
[[nodiscard]] bool layered_error_models_are_distinct() noexcept;

}  // namespace kivo::video::playback_graph::runtime
