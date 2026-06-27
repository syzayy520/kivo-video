// subtitle_packet_safety_policy.hpp — GATE-021: P5A subtitle packet safety policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::attachment {

struct SubtitlePacketSafetyPolicy {
    uint64_t max_packet_bytes{0};           // max subtitle packet size
    bool validate_encoding{true};           // validate packet encoding
    bool reject_embedded_scripts{true};     // reject embedded scripts
    bool reject_external_refs{true};        // reject external references
    std::string rejection_reason;           // reason if packet rejected
    bool operator==(SubtitlePacketSafetyPolicy const&) const = default;
};

}  // namespace kivo::video_plane::attachment
