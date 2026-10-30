#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video::video_plane::probe {

// Numbers a stage must produce to be considered measured rather than claimed.
// Every field is filled from an actual run; none of them are defaults that
// happen to look plausible.
struct DecodeMetrics {
    std::string stage{"p11-a2-decode"};
    std::string input_path;
    std::string codec;
    std::string backend;        // "d3d11va" / "software"
    std::string device_name;
    std::string pixel_format;   // "P010" / "NV12" / "yuv420p10le"
    std::int32_t width{0};
    std::int32_t height{0};
    std::int32_t bit_depth{8};

    std::int32_t decoded_frames{0};
    std::int32_t submitted_packets{0};

    double elapsed_ms{0.0};     // wall time spent inside the decode loop
    double fps{0.0};
    double cpu_time_ms{0.0};
    // Exact CPU cycle count for this run. This is the trustworthy CPU measure:
    // the FILETIME-based timer is quantised and under-reports threaded work here.
    double cpu_cycles{0.0};
    // Filled by the caller when a software baseline exists: cycles_hw / cycles_sw.
    double cpu_ratio{0.0};
    double wall_time_ms{0.0};

    // Per-submission decode step cost, in milliseconds.
    double step_ms_avg{0.0};
    double step_ms_p50{0.0};
    double step_ms_p99{0.0};
    double step_ms_max{0.0};

    bool hardware_decoded{false};
    bool ten_bit{false};
    std::string failure_reason;
};

// Percentile over a sample set already collected by the caller.
// Returns 0 for an empty set. `p` is in [0, 100].
[[nodiscard]] auto percentile(const std::vector<double>& sorted_samples, double p) noexcept
    -> double;

// Formats metrics as machine-readable JSON (single object, stable key order).
[[nodiscard]] auto metrics_to_json(const DecodeMetrics& metrics) -> std::string;

// Formats metrics as a human-readable block for the console.
[[nodiscard]] auto metrics_to_text(const DecodeMetrics& metrics) -> std::string;

// Writes JSON to disk, creating parent directories when needed.
[[nodiscard]] auto write_metrics_file(const std::string& path, const DecodeMetrics& metrics)
    -> bool;

}  // namespace kivo::video::video_plane::probe
