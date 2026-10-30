#pragma once

#include <cstdint>

#include "kivo/cinema_engine/video_plane_gpu/surface/gpu_surface_contract.hpp"

namespace kivo::cinema_engine::video_plane_gpu {

// Color description carried per frame so the presenter never guesses.
// Values mirror the FFmpeg enums numerically but are kept independent here
// to keep this header free of third-party includes.
struct GpuFrameColorInfo {
    std::int32_t primaries{2};      // unspecified by default
    std::int32_t transfer{2};       // unspecified by default
    std::int32_t matrix{2};         // unspecified by default
    std::int32_t range{0};          // 0 = unspecified, 1 = limited, 2 = full
};

// One decoded video frame that lives entirely in GPU memory.
// Deliberately carries no byte buffer: a frame that can be turned into a
// std::vector would reintroduce the CPU copy this plane exists to remove.
struct GpuFrameContract {
    GpuSurfaceContract surface{};
    std::int64_t pts_us{0};
    std::int64_t duration_us{0};
    GpuFrameColorInfo color{};
    bool hardware_decoded{false};
    // Opaque token the producer maps to the owning decode resource.
    // The consumer must hand it back through HardwareDecodeBackend::release_frame.
    std::uint64_t frame_token{0};

    [[nodiscard]] auto is_valid() const noexcept -> bool {
        return surface.is_valid() && frame_token != 0;
    }
};

}  // namespace kivo::cinema_engine::video_plane_gpu
