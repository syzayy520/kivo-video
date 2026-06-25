#include "kivo/cinema_engine/feature_gate_core/fake_feature_flag_service.hpp"

namespace kivo::cinema_engine {

// Feature flag builders
FeatureFlagDefinition FakeFeatureFlagService::build_boolean_feature_flag() {
    FeatureFlagDefinition flag;
    flag.metadata.schema_version = "1.0.0";
    flag.flag_id = "boolean-flag-001";
    flag.flag_name = "Enable HDR Support";
    flag.flag_type = "boolean";
    flag.default_value = "true";
    flag.description = "Enable HDR video playback support";
    flag.owner = "video-team";
    return flag;
}

FeatureFlagDefinition FakeFeatureFlagService::build_percentage_feature_flag() {
    FeatureFlagDefinition flag;
    flag.metadata.schema_version = "1.0.0";
    flag.flag_id = "percentage-flag-001";
    flag.flag_name = "New Audio Engine Rollout";
    flag.flag_type = "percentage";
    flag.default_value = "0.1";
    flag.description = "Rollout new audio engine to 10% of users";
    flag.owner = "audio-team";
    return flag;
}

FeatureFlagDefinition FakeFeatureFlagService::build_variant_feature_flag() {
    FeatureFlagDefinition flag;
    flag.metadata.schema_version = "1.0.0";
    flag.flag_id = "variant-flag-001";
    flag.flag_name = "UI Theme Variant";
    flag.flag_type = "variant";
    flag.default_value = "dark";
    flag.description = "UI theme variant (dark, light, system)";
    flag.owner = "ui-team";
    return flag;
}

// Config default rule builders
ConfigDefaultRule FakeFeatureFlagService::build_strict_config_rule() {
    ConfigDefaultRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "strict-config-001";
    rule.config_key = "max_buffer_size";
    rule.default_value = "1024";
    rule.validation_regex = "^[0-9]+$";
    rule.fallback_value = "512";
    rule.override_allowed = false;
    return rule;
}

ConfigDefaultRule FakeFeatureFlagService::build_relaxed_config_rule() {
    ConfigDefaultRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "relaxed-config-001";
    rule.config_key = "theme_color";
    rule.default_value = "#ffffff";
    rule.validation_regex = "^#[0-9a-fA-F]{6}$";
    rule.fallback_value = "#000000";
    rule.override_allowed = true;
    return rule;
}

// Rollout strategy builders
RolloutStrategy FakeFeatureFlagService::build_percentage_rollout_strategy() {
    RolloutStrategy strategy;
    strategy.metadata.schema_version = "1.0.0";
    strategy.strategy_id = "percentage-rollout-001";
    strategy.strategy_name = "Gradual Feature Rollout";
    strategy.rollout_percentage = 0.25f;
    strategy.target_groups = {"beta-users", "internal-testers"};
    strategy.start_time = 1719312000000;  // 2024-06-25
    strategy.end_time = 1719398400000;    // 2024-06-26
    return strategy;
}

RolloutStrategy FakeFeatureFlagService::build_group_rollout_strategy() {
    RolloutStrategy strategy;
    strategy.metadata.schema_version = "1.0.0";
    strategy.strategy_id = "group-rollout-001";
    strategy.strategy_name = "Group-Based Feature Rollout";
    strategy.rollout_percentage = 1.0f;
    strategy.target_groups = {"enterprise-customers", "premium-users"};
    strategy.start_time = 1719312000000;
    strategy.end_time = 1719484800000;  // 2024-06-27
    return strategy;
}

// Result builders
FeatureFlagDefinition FakeFeatureFlagService::build_enabled_flag_result() {
    FeatureFlagDefinition result;
    result.metadata.schema_version = "1.0.0";
    result.flag_id = "enabled-flag-001";
    result.flag_name = "Enabled Feature";
    result.flag_type = "boolean";
    result.default_value = "true";
    result.description = "Feature is enabled";
    result.owner = "feature-team";
    return result;
}

FeatureFlagDefinition FakeFeatureFlagService::build_disabled_flag_result() {
    FeatureFlagDefinition result;
    result.metadata.schema_version = "1.0.0";
    result.flag_id = "disabled-flag-001";
    result.flag_name = "Disabled Feature";
    result.flag_type = "boolean";
    result.default_value = "false";
    result.description = "Feature is disabled";
    result.owner = "feature-team";
    return result;
}

FeatureFlagDefinition FakeFeatureFlagService::build_percentage_enabled_result() {
    FeatureFlagDefinition result;
    result.metadata.schema_version = "1.0.0";
    result.flag_id = "percentage-enabled-001";
    result.flag_name = "Percentage Enabled Feature";
    result.flag_type = "percentage";
    result.default_value = "0.5";
    result.description = "Feature enabled for 50% of users";
    result.owner = "feature-team";
    return result;
}

FeatureFlagDefinition FakeFeatureFlagService::build_variant_enabled_result() {
    FeatureFlagDefinition result;
    result.metadata.schema_version = "1.0.0";
    result.flag_id = "variant-enabled-001";
    result.flag_name = "Variant Enabled Feature";
    result.flag_type = "variant";
    result.default_value = "experimental";
    result.description = "Feature enabled with experimental variant";
    result.owner = "feature-team";
    return result;
}

RolloutStrategy FakeFeatureFlagService::build_rollout_in_progress_result() {
    RolloutStrategy result;
    result.metadata.schema_version = "1.0.0";
    result.strategy_id = "rollout-in-progress-001";
    result.strategy_name = "Rollout In Progress";
    result.rollout_percentage = 0.5f;
    result.target_groups = {"all-users"};
    result.start_time = 1719312000000;
    result.end_time = 1719484800000;
    return result;
}

}  // namespace kivo::cinema_engine