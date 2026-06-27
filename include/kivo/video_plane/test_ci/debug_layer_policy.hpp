// debug_layer_policy.hpp — GATE-030: P5A debug layer policy
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::test_ci {

struct DebugLayerPolicy {
    bool enable_in_ci{true};                // enable debug layer in CI
    bool enable_in_lab{true};               // enable debug layer in lab
    bool break_on_severity{true};           // break on severe warnings
    uint32_t max_messages_per_frame{1000};  // max debug messages per frame
    std::string output_file;                // debug layer output file
    bool operator==(DebugLayerPolicy const&) const = default;
};

}  // namespace kivo::video_plane::test_ci
