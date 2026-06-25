#include "kivo/cinema_engine/build_core/fake_build_config_service.hpp"

namespace kivo::cinema_engine {

// Build config policy builders
BuildConfigPolicy FakeBuildConfigService::build_release_build_config() {
    BuildConfigPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "release-build-001";
    policy.build_type = "Release";
    policy.reproducible_build = true;
    policy.deterministic_timestamps = true;
    policy.strip_debug_info = true;
    policy.max_build_time_seconds = 600;
    return policy;
}

BuildConfigPolicy FakeBuildConfigService::build_debug_build_config() {
    BuildConfigPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "debug-build-001";
    policy.build_type = "Debug";
    policy.reproducible_build = false;
    policy.deterministic_timestamps = false;
    policy.strip_debug_info = false;
    policy.max_build_time_seconds = 1200;
    return policy;
}

BuildConfigPolicy FakeBuildConfigService::build_reproducible_build_config() {
    BuildConfigPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "reproducible-build-001";
    policy.build_type = "RelWithDebInfo";
    policy.reproducible_build = true;
    policy.deterministic_timestamps = true;
    policy.strip_debug_info = false;
    policy.max_build_time_seconds = 900;
    return policy;
}

}  // namespace kivo::cinema_engine