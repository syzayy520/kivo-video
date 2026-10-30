#pragma once

#include <cstdint>

namespace kivo::cinema_engine::video_plane_gpu {

// Texture formats the video plane is designed to carry.
// NV12 and P010 are the only accepted decode targets: they keep 8-bit and 10-bit
// content in GPU-native layout, so no RGBA conversion happens anywhere in the path.
enum class GpuPixelFormat : std::uint8_t {
    Unknown = 0,
    Nv12,   // 8-bit 4:2:0 semi-planar
    P010,   // 10-bit 4:2:0 semi-planar, high bit depth in the upper bits
    Rgba8,  // diagnostic only; never a decode target
};

// Opaque handle to a GPU texture. Concrete API types stay behind the adapter boundary.
using GpuTextureHandle = void*;

// Description of one texture allocation.
struct GpuSurfaceDesc {
    std::int32_t width{0};
    std::int32_t height{0};
    GpuPixelFormat format{GpuPixelFormat::Unknown};
    // Which array slice of the texture carries this frame.
    std::uint32_t array_slice{0};

    [[nodiscard]] auto is_valid() const noexcept -> bool {
        return width > 0 && height > 0 && format != GpuPixelFormat::Unknown;
    }
};

// A borrowed view of a GPU texture owned by a surface pool.
// Borrowing does not transfer ownership: the pool reclaims the surface on release.
struct GpuSurfaceContract {
    GpuTextureHandle texture{nullptr};
    GpuSurfaceDesc desc{};
    // Monotonic identifier used for leak accounting and evidence output.
    std::uint64_t surface_id{0};

    [[nodiscard]] auto is_valid() const noexcept -> bool {
        return texture != nullptr && desc.is_valid();
    }
};

}  // namespace kivo::cinema_engine::video_plane_gpu
