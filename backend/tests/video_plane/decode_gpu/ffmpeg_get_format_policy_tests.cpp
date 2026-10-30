// Contract test: the get_format policy must never pick a software format.
// A decoder that silently drops to software decode is the exact failure this
// policy exists to prevent, so the refusal path is tested as hard as the happy path.

#include <libavutil/pixfmt.h>

#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_get_format_policy.hpp"

namespace {

using kivo::video::video_plane::backends::d3d11::hwaccel::decide_get_format;

// Hardware format present among software ones: it must be chosen.
auto test_selects_d3d11() -> int {
    const AVPixelFormat offered[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_D3D11, AV_PIX_FMT_NONE};
    const auto decision = decide_get_format(offered);
    if (decision.format != AV_PIX_FMT_D3D11) {
        return 1;
    }
    if (!decision.is_hardware()) {
        return 2;
    }
    if (decision.reason.empty()) {
        return 3;
    }
    return 0;
}

// Only software formats offered: the policy must refuse, not accept a fallback.
auto test_refuses_software_only() -> int {
    const AVPixelFormat offered[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_NONE};
    const auto decision = decide_get_format(offered);
    if (decision.format != AV_PIX_FMT_NONE) {
        return 4;
    }
    if (decision.is_hardware()) {
        return 5;
    }
    if (decision.reason.empty()) {
        return 6;
    }
    return 0;
}

// Degenerate input must still fail closed.
auto test_refuses_null_offer() -> int {
    const auto decision = decide_get_format(nullptr);
    if (decision.format != AV_PIX_FMT_NONE) {
        return 7;
    }
    return 0;
}

// A list containing only the terminator must not be mistaken for hardware support.
auto test_refuses_empty_offer() -> int {
    const AVPixelFormat offered[] = {AV_PIX_FMT_NONE};
    const auto decision = decide_get_format(offered);
    if (decision.format != AV_PIX_FMT_NONE) {
        return 8;
    }
    return 0;
}

}  // namespace

auto main() -> int {
    if (const int rc = test_selects_d3d11(); rc != 0) {
        return rc;
    }
    if (const int rc = test_refuses_software_only(); rc != 0) {
        return rc;
    }
    if (const int rc = test_refuses_null_offer(); rc != 0) {
        return rc;
    }
    if (const int rc = test_refuses_empty_offer(); rc != 0) {
        return rc;
    }
    return 0;
}
