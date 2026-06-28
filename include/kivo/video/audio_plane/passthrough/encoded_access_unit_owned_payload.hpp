// encoded_access_unit_owned_payload.hpp — P6A passthrough: EncodedPayloadKind + EncodedAccessUnitOwnedPayload (§8.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
enum class EncodedPayloadKind {
    PoolSlot = 0,
    CompactBlock = 1,
    BackendOpaque = 2,
};
// Owned payload contract — MUST be owned, never borrowed.
struct EncodedAccessUnitOwnedPayload {
    ContractMetadata metadata;
    EncodedPayloadKind kind{EncodedPayloadKind::PoolSlot};
    uint64_t handle_id{0};
    int64_t size_bytes{0};
    bool is_borrowed{false};  // contract invariant: MUST remain false
    bool operator==(const EncodedAccessUnitOwnedPayload&) const = default;
};
}  // namespace kivo::video::audio_plane::passthrough
