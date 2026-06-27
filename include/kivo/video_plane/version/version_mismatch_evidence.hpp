// version_mismatch_evidence.hpp — GATE-007: P5A version mismatch evidence
#pragma once
#include <cstdint>
#include <string>
#include "boundary_contract_version.hpp"

namespace kivo::video_plane::version {

struct VersionMismatchEvidence {
    BoundaryContractVersion expected_version;
    BoundaryContractVersion actual_version;
    std::string boundary_name;              // which boundary reported mismatch
    std::string evidence_id;
    std::string correlation_id;
    bool operator==(VersionMismatchEvidence const&) const = default;
};

}  // namespace kivo::video_plane::version
