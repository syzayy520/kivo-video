// audio_data_ownership_mode.hpp — P6A §7/§8: 6 ownership variants
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::data {
enum class AudioDataOwnershipMode : int32_t {
    SyncBorrowedByteView = 0,
    MoveOwnedBlock = 1,
    CopiedIntoP6PacketPool = 2,
    PoolSlotOwnedByP6 = 3,
    ExternalOpaque = 4,
    Rejected = 5,
};
enum class AudioDataImportSource : int32_t {
    SyncBorrowedByteView = 0,
    MoveOwnedBlock = 1,
    CopiedIntoP6PacketPool = 2,
    PoolSlotOwnedByP6 = 3,
    ExternalOpaque = 4,
    Rejected = 5,
};
struct AudioDataOwnershipModeContract {
    ContractMetadata metadata;
    bool span_sync_call_only{true};
    bool span_must_not_be_stored{true};
    bool span_must_not_cross_thread{true};
    bool span_must_not_enter_backend_state{true};
    bool span_must_not_embed_in_spsc{true};
    bool span_must_not_embed_in_encoded_access_unit{true};
};
}  // namespace kivo::video::audio_plane::data
