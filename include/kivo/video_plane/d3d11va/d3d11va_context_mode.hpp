// d3d11va_context_mode.hpp — GATE-016: P5A D3D11VA context mode
#pragma once
#include <string>

namespace kivo::video_plane::d3d11va {

enum class D3D11vaContextMode {
    NotInitialized,                         // context not initialized
    Hardware,                               // hardware D3D11VA context
    Software,                               // software fallback
    Failed                                  // context initialization failed
};

struct D3D11vaContext {
    D3D11vaContextMode mode{D3D11vaContextMode::NotInitialized};
    std::string device_description;         // device description string
    uint32_t vendor_id{0};                  // GPU vendor id
    uint32_t device_id{0};                  // GPU device id
    bool operator==(D3D11vaContext const&) const = default;
};

}  // namespace kivo::video_plane::d3d11va
