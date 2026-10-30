#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_get_format_policy.hpp"

namespace kivo::video::video_plane::backends::d3d11::hwaccel {

auto decide_get_format(const AVPixelFormat* offered) noexcept -> GetFormatDecision {
    GetFormatDecision decision{};
    if (offered == nullptr) {
        decision.reason = "get_format called with no candidate formats";
        return decision;
    }

    for (const AVPixelFormat* candidate = offered; *candidate != AV_PIX_FMT_NONE; ++candidate) {
        if (*candidate == AV_PIX_FMT_D3D11) {
            decision.format = AV_PIX_FMT_D3D11;
            decision.reason = "d3d11va texture format offered";
            return decision;
        }
    }

    // No hardware format was offered. Refuse explicitly: a silent switch to
    // software decode here is exactly the failure mode this plane forbids.
    decision.format = AV_PIX_FMT_NONE;
    decision.reason = "no hardware pixel format offered by decoder";
    return decision;
}

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
