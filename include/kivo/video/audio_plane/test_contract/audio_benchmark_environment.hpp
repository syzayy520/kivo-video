// audio_benchmark_environment.hpp — P6A test_contract: benchmark env (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioBenchmarkEnvironment
// Environment descriptor for benchmark runs. Compile-only skeleton.
struct AudioBenchmarkEnvironment {
    kivo::video::audio_plane::ContractMetadata metadata{};
    const char* cpu_model{""};
    int32_t core_count{0};
    const char* windows_version{""};
    const char* power_mode{""};
    int32_t sample_rate{0};
    int32_t channel_count{0};
    int32_t sample_format{0};
    int32_t buffer_size{0};
    int32_t shared_exclusive{0};
    int32_t device_period_us{0};
    int32_t backend_fake_real{0};
    int32_t release_debug{0};
    int32_t sanitizer{0};
    int32_t warmup_duration_ms{0};
    int32_t sample_count{0};

    bool operator==(const AudioBenchmarkEnvironment&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
