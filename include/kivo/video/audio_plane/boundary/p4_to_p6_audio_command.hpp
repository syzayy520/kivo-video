// p4_to_p6_audio_command.hpp — P6A boundary: P4→P6 audio command (§6, §7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P4ToP6AudioCommand
// Command crossing the P4 (orchestration) → P6 (audio media plane) boundary.
// command_kind and priority are enum-as-int placeholders (no platform enums).
// `is_retry` and `retry_of_command_id` support idempotent retry semantics.
struct P4ToP6AudioCommand {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t command_id{0};
    uint64_t command_generation{0};
    int32_t command_kind{0};
    int32_t priority{0};
    bool is_retry{false};
    uint64_t retry_of_command_id{0};

    bool operator==(const P4ToP6AudioCommand&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
