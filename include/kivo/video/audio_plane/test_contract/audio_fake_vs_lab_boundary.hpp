// audio_fake_vs_lab_boundary.hpp — P6A test_contract: fake vs lab (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioFakeVsLabBoundary
// Boundary between fake and lab testing. Compile-only skeleton.
struct AudioFakeVsLabBoundary {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool fake_validates_state_machines{true};
    bool fake_cannot_unlock_production_claim{true};
    bool lab_required_for_positive_device_claim{true};

    bool operator==(const AudioFakeVsLabBoundary&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
