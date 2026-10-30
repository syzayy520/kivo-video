#pragma once

#include <cstdint>
#include <string>

namespace kivo::cinema_engine::video_plane_gpu {

// Hardware capability class of the selected GPU device.
// Warp is the software rasterizer; it must never be reported as hardware decode.
enum class GpuDeviceKind : std::uint8_t {
    Unknown = 0,
    Hardware,
    Warp,
};

// Descriptive identity of the adapter backing the device.
struct GpuAdapterInfo {
    std::string name;
    std::uint32_t vendor_id{0};
    std::uint32_t device_id{0};
    std::uint64_t dedicated_video_memory_bytes{0};
    bool is_software{false};
};

// Opaque handle to the underlying graphics device.
// Concrete API types stay inside the adapter boundary and never cross this header.
using GpuDeviceHandle = void*;

// What the caller needs to know about a created device.
struct GpuDeviceContract {
    GpuDeviceKind kind{GpuDeviceKind::Unknown};
    GpuAdapterInfo adapter{};
    bool video_decode_capable{false};
    bool multithread_protected{false};
};

}  // namespace kivo::cinema_engine::video_plane_gpu
