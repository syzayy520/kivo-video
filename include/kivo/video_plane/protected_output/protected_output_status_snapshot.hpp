// protected_output_status_snapshot.hpp — GATE-012: P5A protected output status snapshot
#pragma once
#include <cstdint>
#include <string>
#include "protected_output_coverage.hpp"
#include "active_output_set.hpp"

namespace kivo::video_plane::protected_output {

struct ProtectedOutputStatusSnapshot {
    ProtectedOutputCoverage coverage;
    ActiveOutputSet active_outputs;
    uint64_t snapshot_timestamp{0};
    std::string evidence_id;
    bool operator==(ProtectedOutputStatusSnapshot const&) const = default;
};

}  // namespace kivo::video_plane::protected_output
