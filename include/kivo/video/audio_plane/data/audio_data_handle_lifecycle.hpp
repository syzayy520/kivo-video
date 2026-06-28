// audio_data_handle_lifecycle.hpp — P6A §8: handle lifecycle states
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::data {
enum class AudioDataHandleLifecycle : int32_t { Created = 0, Imported = 1, InDecode = 2, InRender = 3, Retired = 4, Released = 5 };
struct AudioDataHandleLifecycleContract {
    ContractMetadata metadata;
    bool decode_backend_holds_only_p6_owned{true};
    bool decode_backend_must_not_hold_borrowed_span{true};
    bool decode_backend_returns_on_flush_drain_close{true};
    bool hold_timeout_observable_by_evidence{true};
};
}  // namespace kivo::video::audio_plane::data
