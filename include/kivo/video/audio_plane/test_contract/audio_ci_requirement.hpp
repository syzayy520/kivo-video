// audio_ci_requirement.hpp — P6A test_contract: CI requirement (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioCiRequirement
// CI requirements for the audio plane. Compile-only skeleton.
struct AudioCiRequirement {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool ci_required_pass{true};
    bool arch_guard_pass{true};
    bool forbidden_token_pass{true};

    bool operator==(const AudioCiRequirement&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
