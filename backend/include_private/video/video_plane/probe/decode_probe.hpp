#pragma once

#include <string>

#include "video/video_plane/probe/probe_options.hpp"
#include "video/video_plane/probe/probe_report.hpp"

namespace kivo::video::video_plane::probe {

struct ProbeResult {
    bool ok{false};
    DecodeMetrics metrics{};
    std::string error;
};

// Runs one hardware decode session through the real backend
// (D3d11VideoDevice + FfmpegD3d11vaDecoder) and reports what actually happened.
// It deliberately fails instead of reporting success when the decoder falls back
// to software: a soft frame disguised as a hardware one is worse than an error.
class HardwareDecodeProbe {
public:
    explicit HardwareDecodeProbe(ProbeOptions options) : options_(std::move(options)) {}

    [[nodiscard]] auto run() -> ProbeResult;

private:
    ProbeOptions options_;
};

}  // namespace kivo::video::video_plane::probe
