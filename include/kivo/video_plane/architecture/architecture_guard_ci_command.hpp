// architecture_guard_ci_command.hpp — GATE-026: P5A architecture guard CI command
#pragma once
#include <string>
#include <vector>

namespace kivo::video_plane::architecture {

// Local command:
//   cmake --build --preset ninja-debug --target kivo_video_plane_contracts
//   cmake --build --preset ninja-debug --target kivo_video_plane_core
//   powershell -File backend/tools/governance/check_p5_architecture_guard.ps1
//
// CI command:
//   same as local, plus:
//   dumpbin /exports kivo_video_plane_contracts.lib | findstr /V "kivo::video_plane::"

struct PositiveTest {
    std::string test_name;
    std::string expected_result;
};

struct NegativeTest {
    std::string test_name;
    std::string expected_violation;
};

struct ArchitectureGuardCiCommand {
    static std::vector<PositiveTest> positive_tests();
    static std::vector<NegativeTest> negative_tests();
};

}  // namespace kivo::video_plane::architecture
