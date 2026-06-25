#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/testing_core/fake_test_oracle_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_strict_hermetic_requirement() {
    auto req = FakeTestOracleService::build_strict_hermetic_requirement();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "strict-hermetic-001");
    assert(req.fake_type == "strict");
    assert(req.isolation_level == "process");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == true);
    assert(req.description == "Strict hermetic requirement with process isolation and state reset");
}

void test_relaxed_hermetic_requirement() {
    auto req = FakeTestOracleService::build_relaxed_hermetic_requirement();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "relaxed-hermetic-001");
    assert(req.fake_type == "relaxed");
    assert(req.isolation_level == "thread");
    assert(req.state_reset_required == false);
    assert(req.dependency_injection == false);
    assert(req.description == "Relaxed hermetic requirement with thread isolation and no state reset");
}

void test_process_isolated_requirement() {
    auto req = FakeTestOracleService::build_process_isolated_requirement();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "process-isolated-001");
    assert(req.fake_type == "mock");
    assert(req.isolation_level == "process");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == true);
    assert(req.description == "Process-isolated requirement for complete test isolation");
}

void test_thread_isolated_requirement() {
    auto req = FakeTestOracleService::build_thread_isolated_requirement();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "thread-isolated-001");
    assert(req.fake_type == "stub");
    assert(req.isolation_level == "thread");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == true);
    assert(req.description == "Thread-isolated requirement for lightweight test isolation");
}

void test_gpu_hardware_pair_policy() {
    auto policy = FakeTestOracleService::build_gpu_hardware_pair_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "gpu-hardware-pair-001");
    assert(policy.hardware_type == "GPU");
    assert(policy.pairing_required == true);
    assert(policy.manual_override_allowed == false);
    assert(policy.fallback_to_fake == true);
    assert(policy.timeout_ms == 5000);
}

void test_audio_hardware_pair_policy() {
    auto policy = FakeTestOracleService::build_audio_hardware_pair_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "audio-hardware-pair-001");
    assert(policy.hardware_type == "audio");
    assert(policy.pairing_required == true);
    assert(policy.manual_override_allowed == true);
    assert(policy.fallback_to_fake == true);
    assert(policy.timeout_ms == 3000);
}

void test_strict_reproducibility_rule() {
    auto rule = FakeTestOracleService::build_strict_reproducibility_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "strict-reproducibility-001");
    assert(rule.rule_name == "Strict Deterministic Test Rule");
    assert(rule.deterministic_required == true);
    assert(rule.seed_control == true);
    assert(rule.time_mocking == true);
    assert(rule.environment_isolation == true);
}

void test_relaxed_reproducibility_rule() {
    auto rule = FakeTestOracleService::build_relaxed_reproducibility_rule();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "relaxed-reproducibility-001");
    assert(rule.rule_name == "Relaxed Deterministic Test Rule");
    assert(rule.deterministic_required == false);
    assert(rule.seed_control == false);
    assert(rule.time_mocking == false);
    assert(rule.environment_isolation == false);
}

void test_deterministic_test_result() {
    auto req = FakeTestOracleService::build_deterministic_test_result();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "deterministic-result-001");
    assert(req.fake_type == "fake");
    assert(req.isolation_level == "function");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == true);
    assert(req.description == "Deterministic test result requirement");
}

void test_seed_controlled_test_result() {
    auto req = FakeTestOracleService::build_seed_controlled_test_result();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "seed-controlled-result-001");
    assert(req.fake_type == "spy");
    assert(req.isolation_level == "thread");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == false);
    assert(req.description == "Seed controlled test result requirement");
}

void test_time_mocked_test_result() {
    auto req = FakeTestOracleService::build_time_mocked_test_result();
    assert(req.metadata.schema_version == "1.0.0");
    assert(req.requirement_id == "time-mocked-result-001");
    assert(req.fake_type == "mock");
    assert(req.isolation_level == "process");
    assert(req.state_reset_required == true);
    assert(req.dependency_injection == true);
    assert(req.description == "Time mocked test result requirement");
}

void test_environment_isolated_test_result() {
    auto rule = FakeTestOracleService::build_environment_isolated_test_result();
    assert(rule.metadata.schema_version == "1.0.0");
    assert(rule.rule_id == "environment-isolated-result-001");
    assert(rule.rule_name == "Environment Isolated Test Result Rule");
    assert(rule.deterministic_required == true);
    assert(rule.seed_control == true);
    assert(rule.time_mocking == false);
    assert(rule.environment_isolation == true);
}

}  // namespace

int main() {
    std::cout << "Running P2-050 Test Oracle / Reproducibility tests..." << std::endl;

    test_strict_hermetic_requirement();
    std::cout << "PASS: test_strict_hermetic_requirement" << std::endl;

    test_relaxed_hermetic_requirement();
    std::cout << "PASS: test_relaxed_hermetic_requirement" << std::endl;

    test_process_isolated_requirement();
    std::cout << "PASS: test_process_isolated_requirement" << std::endl;

    test_thread_isolated_requirement();
    std::cout << "PASS: test_thread_isolated_requirement" << std::endl;

    test_gpu_hardware_pair_policy();
    std::cout << "PASS: test_gpu_hardware_pair_policy" << std::endl;

    test_audio_hardware_pair_policy();
    std::cout << "PASS: test_audio_hardware_pair_policy" << std::endl;

    test_strict_reproducibility_rule();
    std::cout << "PASS: test_strict_reproducibility_rule" << std::endl;

    test_relaxed_reproducibility_rule();
    std::cout << "PASS: test_relaxed_reproducibility_rule" << std::endl;

    test_deterministic_test_result();
    std::cout << "PASS: test_deterministic_test_result" << std::endl;

    test_seed_controlled_test_result();
    std::cout << "PASS: test_seed_controlled_test_result" << std::endl;

    test_time_mocked_test_result();
    std::cout << "PASS: test_time_mocked_test_result" << std::endl;

    test_environment_isolated_test_result();
    std::cout << "PASS: test_environment_isolated_test_result" << std::endl;

    std::cout << "\nAll 12 P2-050 tests passed!" << std::endl;
    return 0;
}