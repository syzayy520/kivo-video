// iec61937_packetization_policy.hpp — P6A passthrough: Iec61937PacketizationPolicy (§8.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
struct Iec61937PacketizationPolicy {
    ContractMetadata metadata;
    bool p6a_defines_policy_shape_only{true};
    bool p6f_adds_constants_with_fixture{true};
    bool operator==(const Iec61937PacketizationPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
