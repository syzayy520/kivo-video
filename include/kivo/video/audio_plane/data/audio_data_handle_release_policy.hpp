// audio_data_handle_release_policy.hpp — P6A §8: release responsibility
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::data {
struct AudioDataHandleReleasePolicy {
    ContractMetadata metadata;
    bool rejected_before_transfer_original_owner_releases{true};
    bool rejected_after_move_p6_releases{true};
    bool deferred_ownership_remains_with_sender{true};
    bool superseded_p6_retires_by_generation{true};
    bool heavy_release_not_on_device_thread{true};
    bool deferred_move_owned_forbidden{true};
};
}  // namespace kivo::video::audio_plane::data
