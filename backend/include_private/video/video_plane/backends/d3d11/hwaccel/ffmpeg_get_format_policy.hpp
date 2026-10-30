#pragma once

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <string>

namespace kivo::video::video_plane::backends::d3d11::hwaccel {

// Decision produced for FFmpeg's get_format callback.
// `format` is AV_PIX_FMT_NONE when no hardware format is on offer; returning
// NONE makes FFmpeg fail the open instead of silently falling back to software.
struct GetFormatDecision {
    AVPixelFormat format{AV_PIX_FMT_NONE};
    std::string reason;

    [[nodiscard]] auto is_hardware() const noexcept -> bool {
        return format == AV_PIX_FMT_D3D11;
    }
};

// Pure selection policy: given the formats FFmpeg offers, choose the hardware
// one. Only AV_PIX_FMT_D3D11 is accepted; nothing else qualifies as hardware.
[[nodiscard]] auto decide_get_format(const AVPixelFormat* offered) noexcept -> GetFormatDecision;

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
