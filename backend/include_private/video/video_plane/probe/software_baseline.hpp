#pragma once

#include <string>

#include "video/video_plane/probe/probe_options.hpp"
#include "video/video_plane/probe/probe_report.hpp"

namespace kivo::video::video_plane::probe {

// Reference measurement: decode the same file purely on the CPU.
// Its only purpose is to give the hardware number a denominator, so "hardware
// decode is faster" becomes "hardware decode costs N% of the CPU".
[[nodiscard]] auto measure_software_baseline(const ProbeOptions& options,
                                             DecodeMetrics& out,
                                             std::string& error) -> bool;

}  // namespace kivo::video::video_plane::probe
