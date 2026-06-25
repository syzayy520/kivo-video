#pragma once

#include "kivo/cinema_engine/testing_core/hermetic_fake_requirement.hpp"
#include "kivo/cinema_engine/testing_core/hardware_pair_policy.hpp"
#include "kivo/cinema_engine/testing_core/test_reproducibility_rule.hpp"

namespace kivo::cinema_engine {

// Fake test oracle service for testing.
// Provides static builder functions for hermetic fakes, hardware pairs, and reproducibility rules.
class FakeTestOracleService {
 public:
    // Hermetic fake requirement builders
    static HermeticFakeRequirement build_strict_hermetic_requirement();
    static HermeticFakeRequirement build_relaxed_hermetic_requirement();
    static HermeticFakeRequirement build_process_isolated_requirement();
    static HermeticFakeRequirement build_thread_isolated_requirement();
    
    // Hardware pair policy builders
    static HardwarePairPolicy build_gpu_hardware_pair_policy();
    static HardwarePairPolicy build_audio_hardware_pair_policy();
    
    // Test reproducibility rule builders
    static TestReproducibilityRule build_strict_reproducibility_rule();
    static TestReproducibilityRule build_relaxed_reproducibility_rule();
    
    // Result builders (using existing types as result types for simplicity)
    static HermeticFakeRequirement build_deterministic_test_result();
    static HermeticFakeRequirement build_seed_controlled_test_result();
    static HermeticFakeRequirement build_time_mocked_test_result();
    static TestReproducibilityRule build_environment_isolated_test_result();
};

}  // namespace kivo::cinema_engine