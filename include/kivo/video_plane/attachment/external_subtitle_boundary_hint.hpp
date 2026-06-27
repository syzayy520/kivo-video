// external_subtitle_boundary_hint.hpp — GATE-021: P5A external subtitle boundary hint
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::attachment {

struct ExternalSubtitleBoundaryHint {
    std::string subtitle_path;              // external subtitle file path
    std::string detected_encoding;          // detected encoding
    bool is_text_subtitle{true};            // text-based subtitle
    bool requires_sniffing{true};           // requires MIME sniffing
    bool operator==(ExternalSubtitleBoundaryHint const&) const = default;
};

}  // namespace kivo::video_plane::attachment
