// atmos_object_audio_scope.hpp — P6A security: Atmos object audio scope (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AtmosObjectAudioScope
// Scope of Atmos object audio handling. Compile-only skeleton.
struct AtmosObjectAudioScope {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool metadata_detected{false};
    bool passthrough_active{false};
    bool object_rendering_supported{false};
    bool baseline_no_object_rendering{true};

    bool operator==(const AtmosObjectAudioScope&) const = default;
};

}  // namespace kivo::video::audio_plane::security
