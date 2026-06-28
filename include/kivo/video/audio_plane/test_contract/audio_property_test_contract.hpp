// audio_property_test_contract.hpp — P6A test_contract: property test (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioPropertyTestContract
// Contract for property tests. Compile-only skeleton.
struct AudioPropertyTestContract {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t run_count{100};

    bool operator==(const AudioPropertyTestContract&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
