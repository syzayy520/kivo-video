#include "kivo/cinema_engine/testing_core/fake_test_oracle_service.hpp"

namespace kivo::cinema_engine {

// Hermetic fake requirement builders
HermeticFakeRequirement FakeTestOracleService::build_strict_hermetic_requirement() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "strict-hermetic-001";
    req.fake_type = "strict";
    req.isolation_level = "process";
    req.state_reset_required = true;
    req.dependency_injection = true;
    req.description = "Strict hermetic requirement with process isolation and state reset";
    return req;
}

HermeticFakeRequirement FakeTestOracleService::build_relaxed_hermetic_requirement() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "relaxed-hermetic-001";
    req.fake_type = "relaxed";
    req.isolation_level = "thread";
    req.state_reset_required = false;
    req.dependency_injection = false;
    req.description = "Relaxed hermetic requirement with thread isolation and no state reset";
    return req;
}

HermeticFakeRequirement FakeTestOracleService::build_process_isolated_requirement() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "process-isolated-001";
    req.fake_type = "mock";
    req.isolation_level = "process";
    req.state_reset_required = true;
    req.dependency_injection = true;
    req.description = "Process-isolated requirement for complete test isolation";
    return req;
}

HermeticFakeRequirement FakeTestOracleService::build_thread_isolated_requirement() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "thread-isolated-001";
    req.fake_type = "stub";
    req.isolation_level = "thread";
    req.state_reset_required = true;
    req.dependency_injection = true;
    req.description = "Thread-isolated requirement for lightweight test isolation";
    return req;
}

// Hardware pair policy builders
HardwarePairPolicy FakeTestOracleService::build_gpu_hardware_pair_policy() {
    HardwarePairPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "gpu-hardware-pair-001";
    policy.hardware_type = "GPU";
    policy.pairing_required = true;
    policy.manual_override_allowed = false;
    policy.fallback_to_fake = true;
    policy.timeout_ms = 5000;
    return policy;
}

HardwarePairPolicy FakeTestOracleService::build_audio_hardware_pair_policy() {
    HardwarePairPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "audio-hardware-pair-001";
    policy.hardware_type = "audio";
    policy.pairing_required = true;
    policy.manual_override_allowed = true;
    policy.fallback_to_fake = true;
    policy.timeout_ms = 3000;
    return policy;
}

// Test reproducibility rule builders
TestReproducibilityRule FakeTestOracleService::build_strict_reproducibility_rule() {
    TestReproducibilityRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "strict-reproducibility-001";
    rule.rule_name = "Strict Deterministic Test Rule";
    rule.deterministic_required = true;
    rule.seed_control = true;
    rule.time_mocking = true;
    rule.environment_isolation = true;
    return rule;
}

TestReproducibilityRule FakeTestOracleService::build_relaxed_reproducibility_rule() {
    TestReproducibilityRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "relaxed-reproducibility-001";
    rule.rule_name = "Relaxed Deterministic Test Rule";
    rule.deterministic_required = false;
    rule.seed_control = false;
    rule.time_mocking = false;
    rule.environment_isolation = false;
    return rule;
}

// Result builders
HermeticFakeRequirement FakeTestOracleService::build_deterministic_test_result() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "deterministic-result-001";
    req.fake_type = "fake";
    req.isolation_level = "function";
    req.state_reset_required = true;
    req.dependency_injection = true;
    req.description = "Deterministic test result requirement";
    return req;
}

HermeticFakeRequirement FakeTestOracleService::build_seed_controlled_test_result() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "seed-controlled-result-001";
    req.fake_type = "spy";
    req.isolation_level = "thread";
    req.state_reset_required = true;
    req.dependency_injection = false;
    req.description = "Seed controlled test result requirement";
    return req;
}

HermeticFakeRequirement FakeTestOracleService::build_time_mocked_test_result() {
    HermeticFakeRequirement req;
    req.metadata.schema_version = "1.0.0";
    req.requirement_id = "time-mocked-result-001";
    req.fake_type = "mock";
    req.isolation_level = "process";
    req.state_reset_required = true;
    req.dependency_injection = true;
    req.description = "Time mocked test result requirement";
    return req;
}

TestReproducibilityRule FakeTestOracleService::build_environment_isolated_test_result() {
    TestReproducibilityRule rule;
    rule.metadata.schema_version = "1.0.0";
    rule.rule_id = "environment-isolated-result-001";
    rule.rule_name = "Environment Isolated Test Result Rule";
    rule.deterministic_required = true;
    rule.seed_control = true;
    rule.time_mocking = false;
    rule.environment_isolation = true;
    return rule;
}

}  // namespace kivo::cinema_engine