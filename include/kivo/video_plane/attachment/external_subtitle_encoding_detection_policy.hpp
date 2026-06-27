// external_subtitle_encoding_detection_policy.hpp — GATE-021: P5A external subtitle encoding detection policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::attachment {

enum class EncodingDetectionMode {
    None,                                   // no detection
    BOMOnly,                                // detect by BOM only
    Heuristic,                              // heuristic detection
    Strict                                  // strict detection with validation
};

struct ExternalSubtitleEncodingDetectionPolicy {
    EncodingDetectionMode mode{EncodingDetectionMode::Heuristic};
    uint32_t max_probe_bytes{4096};         // max bytes to probe for encoding
    std::string fallback_encoding;          // fallback encoding if detection fails
    bool reject_unknown_encoding{false};    // reject if encoding unknown
    bool operator==(ExternalSubtitleEncodingDetectionPolicy const&) const = default;
};

}  // namespace kivo::video_plane::attachment
