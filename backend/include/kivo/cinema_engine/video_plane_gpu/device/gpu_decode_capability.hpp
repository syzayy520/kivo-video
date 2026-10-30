#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine::video_plane_gpu {

// Video codecs this engine is allowed to advertise hardware support for.
// The list is closed on purpose: no codec may be claimed without an explicit entry.
enum class GpuCodec : std::uint8_t {
    Unknown = 0,
    H264,
    Hevc,
    Av1,
    Vp9,
    Vc1,
    Mpeg2,
};

// Bit depths a codec is hardware-accelerated at.
// Ten-bit is tracked separately because HDR delivery depends on it.
struct GpuCodecSupport {
    GpuCodec codec{GpuCodec::Unknown};
    bool eight_bit{false};
    bool ten_bit{false};
    bool twelve_bit{false};
};

// Aggregate decode capability of a device.
// reason_if_unavailable must be non-empty whenever hardware_decode_available is false,
// so callers can surface a truthful degradation reason instead of failing silently.
struct GpuDecodeCapability {
    bool hardware_decode_available{false};
    std::string reason_if_unavailable;
    std::string decoder_name;
    std::vector<GpuCodecSupport> codecs;

    [[nodiscard]] auto supports(GpuCodec codec, bool ten_bit) const noexcept -> bool {
        for (const auto& entry : codecs) {
            if (entry.codec != codec) {
                continue;
            }
            return ten_bit ? entry.ten_bit : entry.eight_bit;
        }
        return false;
    }
};

}  // namespace kivo::cinema_engine::video_plane_gpu
