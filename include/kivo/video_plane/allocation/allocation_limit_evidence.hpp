// allocation_limit_evidence.hpp — GATE-005: P5A allocation limit exceeded evidence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::allocation {

struct AllocationLimitExceededEvidence {
    std::string limit_type;                 // which limit was exceeded
    uint64_t limit_value{0};
    uint64_t actual_value{0};
    std::string source_id;
    std::string evidence_id;
    bool operator==(AllocationLimitExceededEvidence const&) const = default;
};

}  // namespace kivo::video_plane::allocation
