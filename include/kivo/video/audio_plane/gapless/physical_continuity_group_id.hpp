// physical_continuity_group_id.hpp — P6A gapless: physical continuity group id value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct PhysicalContinuityGroupId {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(const PhysicalContinuityGroupId&) const = default;
};
}  // namespace kivo::video::audio_plane::gapless
