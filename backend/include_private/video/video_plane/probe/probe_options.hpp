#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::video_plane::probe {

// What the operator asked the probe to do.
// Kept free of any FFmpeg or platform type so the same options parse on every
// target (Android/iOS logging will reuse this).
struct ProbeOptions {
    std::string input_path;
    std::int32_t max_frames{300};
    // Also measure a pure software decode of the same file, so the hardware
    // CPU saving can be stated as a ratio instead of an absolute number.
    bool compare_software{false};
    // Empty means "do not write evidence". A path means "write JSON here".
    std::string evidence_path;
};

// Returns false and fills `error` on misuse; never throws.
[[nodiscard]] auto parse_probe_options(int argc, char** argv, ProbeOptions& out, std::string& error)
    -> bool;

[[nodiscard]] auto probe_usage() -> const char*;

}  // namespace kivo::video::video_plane::probe
