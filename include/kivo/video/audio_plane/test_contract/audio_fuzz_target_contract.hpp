// audio_fuzz_target_contract.hpp — P6A test_contract: fuzz target (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioFuzzTargetContract
// Contract for fuzz targets. Compile-only skeleton.
struct AudioFuzzTargetContract {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t target_count{0};
    bool zero_untriaged_crash{true};

    bool operator==(const AudioFuzzTargetContract&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
