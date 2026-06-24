#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Subtitle overlay contract describing the overlay rendering backend.
// Captures capabilities like HDR luminance-aware rendering and
// ASS advanced feature support.
struct SubtitleOverlayContract {
    ContractMetadata metadata;
    std::string overlay_backend;  // "d3d11_texture", "software_blit"
    bool supports_hdr_luminance{false};
    bool supports_ass_advanced{false};
    std::string font_render_backend;
};

}  // namespace kivo::cinema_engine
