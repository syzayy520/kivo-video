// architecture_target_graph.hpp — GATE-026: P5A architecture target graph
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::architecture {

// 8 CMake targets: contracts, core, ffmpeg_demux, ffmpeg_decode,
//                  d3d11_gpu, dxgi_swapchain, contract_tests, future_backends

struct ArchitectureTargetGraph {
    std::vector<std::string> target_names;  // all 8 target names
    // Link Rules (see §2)
    // Symbol checker rules (see symbol_checker_rule.hpp)
    // Include boundary rules (see include_boundary_checker.hpp)
    bool operator==(ArchitectureTargetGraph const&) const = default;
};

}  // namespace kivo::video_plane::architecture
