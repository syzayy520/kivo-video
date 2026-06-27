// include_boundary_checker.hpp — GATE-026: P5A include boundary checker
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::architecture {

// Input: path to scan (include/kivo/video_plane/)
// Rules:
//   - No #include <windows.h> in any contract header
//   - No #include <d3d11.h> / <dxgi.h> in contracts or core
//   - No #include <libavformat/*> in contracts or core
//   - No #include <Qt/*> in contracts or core
//   - No #include <mmdeviceapi.h> / <audioclient.h> in contracts or core
// Output: list of violations with file:line

struct IncludeBoundaryViolation {
    std::string file_path;
    uint32_t line{0};
    std::string forbidden_include;
    std::string target;
};

struct IncludeBoundaryChecker {
    static std::vector<IncludeBoundaryViolation> scan(std::string const& root_path);
};

}  // namespace kivo::video_plane::architecture
