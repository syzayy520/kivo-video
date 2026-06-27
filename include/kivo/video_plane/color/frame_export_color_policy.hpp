// frame_export_color_policy.hpp — GATE-019: P5A frame export color policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::color {

enum class FrameExportColorSpace {
    sRGB,                                   // export in sRGB
    Rec709,                                 // export in Rec.709
    Rec2020,                                // export in Rec.2020
    PQ,                                     // export in PQ (HDR)
    HLG,                                    // export in HLG (HDR)
    Linear                                  // export in linear
};

struct FrameExportColorPolicy {
    FrameExportColorSpace color_space{FrameExportColorSpace::sRGB};
    bool tone_map_required{false};          // tone mapping required for export
    bool strip_hdr_metadata{false};         // strip HDR metadata on export
    std::string reason;                     // reason for policy choice
    bool operator==(FrameExportColorPolicy const&) const = default;
};

}  // namespace kivo::video_plane::color
