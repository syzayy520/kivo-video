// opaque_metadata_policy.hpp — GATE-014: P5A opaque metadata policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::hdr10_plus {

struct OpaqueMetadataPolicy {
    bool pass_through{true};                // pass metadata through without parsing
    uint64_t max_metadata_bytes{0};         // max metadata blob size
    bool validate_checksum{false};          // validate metadata checksum
    std::string rejection_reason;           // reason if metadata rejected
    bool operator==(OpaqueMetadataPolicy const&) const = default;
};

}  // namespace kivo::video_plane::hdr10_plus
