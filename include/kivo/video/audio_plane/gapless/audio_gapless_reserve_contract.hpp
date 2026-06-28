// audio_gapless_reserve_contract.hpp — P6A gapless: reserve contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::gapless {
struct AudioGaplessReserveContract {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t target_ms{500};
    int32_t minimum_ms{100};
    int32_t maximum_ms{2000};
    bool memory_budget_overrides_duration{true};
    bool p6_cannot_choose_next_track{true};
    bool p6_cannot_fetch_source_data{true};
};
}  // namespace kivo::video::audio_plane::gapless
