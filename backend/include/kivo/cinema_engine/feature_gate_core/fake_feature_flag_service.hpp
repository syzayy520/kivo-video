#pragma once

#include "kivo/cinema_engine/feature_gate_core/feature_flag_definition.hpp"
#include "kivo/cinema_engine/feature_gate_core/config_default_rule.hpp"
#include "kivo/cinema_engine/feature_gate_core/rollout_strategy.hpp"

namespace kivo::cinema_engine {

// Fake feature flag service for testing.
// Provides static builder functions for feature flags, config defaults, and rollout strategies.
class FakeFeatureFlagService {
 public:
    // Feature flag builders
    static FeatureFlagDefinition build_boolean_feature_flag();
    static FeatureFlagDefinition build_percentage_feature_flag();
    static FeatureFlagDefinition build_variant_feature_flag();
    
    // Config default rule builders
    static ConfigDefaultRule build_strict_config_rule();
    static ConfigDefaultRule build_relaxed_config_rule();
    
    // Rollout strategy builders
    static RolloutStrategy build_percentage_rollout_strategy();
    static RolloutStrategy build_group_rollout_strategy();
    
    // Result builders (using FeatureFlagDefinition as result type for simplicity)
    static FeatureFlagDefinition build_enabled_flag_result();
    static FeatureFlagDefinition build_disabled_flag_result();
    static FeatureFlagDefinition build_percentage_enabled_result();
    static FeatureFlagDefinition build_variant_enabled_result();
    static RolloutStrategy build_rollout_in_progress_result();
};

}  // namespace kivo::cinema_engine