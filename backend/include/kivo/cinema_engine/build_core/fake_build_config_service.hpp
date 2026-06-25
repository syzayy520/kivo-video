#pragma once

#include "kivo/cinema_engine/build_core/build_config_policy.hpp"

namespace kivo::cinema_engine {

// Fake build config service for testing.
// Provides static builder functions for build configuration policies.
class FakeBuildConfigService {
 public:
    // Build config policy builders
    static BuildConfigPolicy build_release_build_config();
    static BuildConfigPolicy build_debug_build_config();
    static BuildConfigPolicy build_reproducible_build_config();
};

}  // namespace kivo::cinema_engine