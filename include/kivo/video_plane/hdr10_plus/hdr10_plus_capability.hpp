// hdr10_plus_capability.hpp — GATE-014: P5A HDR10+ capability
#pragma once
#include <string>

namespace kivo::video_plane::hdr10_plus {

enum class Hdr10PlusSupportState {
    NotChecked,                             // not yet checked
    Supported,                              // HDR10+ supported
    NotSupported,                           // HDR10+ not supported
    Failed                                  // capability check failed
};

struct Hdr10PlusCapability {
    Hdr10PlusSupportState state{Hdr10PlusSupportState::NotChecked};
    std::string detected_version;           // detected HDR10+ version
    bool dynamic_metadata_supported{false}; // dynamic metadata supported
    bool operator==(Hdr10PlusCapability const&) const = default;
};

}  // namespace kivo::video_plane::hdr10_plus
