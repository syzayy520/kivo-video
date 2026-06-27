// d3d11_texture_opaque_ref.hpp — GATE-016: P5A D3D11 texture opaque reference
#pragma once
#include <cstdint>

namespace kivo::video_plane::d3d11va {

struct D3D11TextureOpaqueRef {
    uint64_t texture_handle{0};             // opaque texture handle (NOT a COM pointer)
    uint64_t ref_count{0};                  // reference count
    bool is_shared{false};                  // texture is shared across contexts
    bool operator==(D3D11TextureOpaqueRef const&) const = default;
};

}  // namespace kivo::video_plane::d3d11va
