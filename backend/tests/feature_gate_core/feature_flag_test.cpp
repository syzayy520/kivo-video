#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/feature_gate_core/fake_feature_flag_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_boolean_feature_flag() {
    auto flag = FakeFeatureFlagService::build_boolean_feature_flag();
    assert(flag.metadata.schema_version == "1.0.0");
    assert(flag.flag_id == "boolean-flag-001");
    assert(flag.flag_name == "Enable HDR Support");
    assert(flag.flag_type == "boolean");
    assert(flag.default_value == "true");
    assert(flag.description == "Enable HDR video playback support");
    assert(flag.owner == "video-team");
}

void test_percentage_feature_flag() {
    auto flag = FakeFeatureFlagService::build_percentage_feature_flag();
    assert(flag.metadata.schema_version == "1.0.0");
    assert(flag.flag_id == "percentage-flag-001");
    assert(flag.flag_name == "New Audio Engine Rollout");
    assert(flag.flag_type == "percentage");
    assert(flag.default_value == "0.1");
    assert(flag.description == "Rollout new audio engine to 10% of users");
    assert(flag.owner == "audio-team");
}

void test_variant_feature_flag() {
    auto flag = FakeFeatureFlagService::build_variant_feature_flag();
    assert(flag.metadata.schema_version == "1.0.0");
    assert(flag.flag_id == "variant-flag-001");
    assert(flag.flag_name == "UI Theme Variant");
    assert(flag.flag_type == "variant");
    assert(flag.default_value == "dark");
    assert(flag.description == "UI theme variant (dark, light, system)");
    assert(flag.owner == "ui-team");
}

void test_strict_config_rule() {
    auto rule = FakeFeatureFlagService::build_strict_config_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "strict-config-001");
    assert(rule.config_key == "max_buffer_size");
    assert(rule.default_value == "1024");
    assert(rule.validation_regex == "^[0-9]+$");
    assert(rule.fallback_value == "512");
    assert(rule.override_allowed == false);
}

void test_relaxed_config_rule() {
    auto rule = FakeFeatureFlagService::build_relaxed_config_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "relaxed-config-001");
    assert(rule.config_key == "theme_color");
    assert(rule.default_value == "#ffffff");
    assert(rule.validation_regex == "^#[0-9a-fA-F]{6}$");
    assert(rule.fallback_value == "#000000");
    assert(rule.override_allowed == true);
}

void test_percentage_rollout_strategy() {
    auto strategy = FakeFeatureFlagService::build_percentage_rollout_strategy();
    assert(strategy.metadata.schema_version == "1.0.0");
    assert(strategy.strategy_id == "percentage-rollout-001");
    assert(strategy.strategy_name == "Gradual Feature Rollout");
    assert(strategy.rollout_percentage == 0.25f);
    assert(strategy.target_groups.size() == 2);
    assert(strategy.start_time == 1719312000000);
    assert(strategy.end_time == 1719398400000);
}

void test_group_rollout_strategy() {
    auto strategy = FakeFeatureFlagService::build_group_rollout_strategy();
    assert(strategy.metadata.schema_version == "1.0.0");
    assert(strategy.strategy_id == "group-rollout-001");
    assert(strategy.strategy_name == "Group-Based Feature Rollout");
    assert(strategy.rollout_percentage == 1.0f);
    assert(strategy.target_groups.size() == 2);
    assert(strategy.start_time == 1719312000000);
    assert(strategy.end_time == 1719484800000);
}

void test_enabled_flag_result() {
    auto result = FakeFeatureFlagService::build_enabled_flag_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.flag_id == "enabled-flag-001");
    assert(result.flag_name == "Enabled Feature");
    assert(result.flag_type == "boolean");
    assert(result.default_value == "true");
    assert(result.description == "Feature is enabled");
    assert(result.owner == "feature-team");
}

void test_disabled_flag_result() {
    auto result = FakeFeatureFlagService::build_disabled_flag_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.flag_id == "disabled-flag-001");
    assert(result.flag_name == "Disabled Feature");
    assert(result.flag_type == "boolean");
    assert(result.default_value == "false");
    assert(result.description == "Feature is disabled");
    assert(result.owner == "feature-team");
}

void test_percentage_enabled_result() {
    auto result = FakeFeatureFlagService::build_percentage_enabled_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.flag_id == "percentage-enabled-001");
    assert(result.flag_name == "Percentage Enabled Feature");
    assert(result.flag_type == "percentage");
    assert(result.default_value == "0.5");
    assert(result.description == "Feature enabled for 50% of users");
    assert(result.owner == "feature-team");
}

void test_variant_enabled_result() {
    auto result = FakeFeatureFlagService::build_variant_enabled_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.flag_id == "variant-enabled-001");
    assert(result.flag_name == "Variant Enabled Feature");
    assert(result.flag_type == "variant");
    assert(result.default_value == "experimental");
    assert(result.description == "Feature enabled with experimental variant");
    assert(result.owner == "feature-team");
}

void test_rollout_in_progress_result() {
    auto result = FakeFeatureFlagService::build_rollout_in_progress_result();
    assert(result.metadata.schema_version == "1.0.0");
    assert(result.strategy_id == "rollout-in-progress-001");
    assert(result.strategy_name == "Rollout In Progress");
    assert(result.rollout_percentage == 0.5f);
    assert(result.target_groups.size() == 1);
    assert(result.start_time == 1719312000000);
    assert(result.end_time == 1719484800000);
}

}  // namespace

int main() {
    std::cout << "Running feature flag tests..." << std::endl;
    
    test_boolean_feature_flag();
    test_percentage_feature_flag();
    test_variant_feature_flag();
    test_strict_config_rule();
    test_relaxed_config_rule();
    test_percentage_rollout_strategy();
    test_group_rollout_strategy();
    test_enabled_flag_result();
    test_disabled_flag_result();
    test_percentage_enabled_result();
    test_variant_enabled_result();
    test_rollout_in_progress_result();
    
    std::cout << "All 12 feature flag tests passed." << std::endl;
    return 0;
}