// p6_to_p4_audio_event.hpp — P6A boundary: P6→P4 audio event (§6, §7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::boundary {

// P6ToP4AudioEvent
// Event flowing from the P6 audio media plane up to the P4 orchestration layer.
// event_kind is an enum-as-int placeholder. engine_epoch and evidence_id carry
// the genealogy/evidence chain for observability without runtime coupling.
struct P6ToP4AudioEvent {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t event_id{0};
    int32_t event_kind{0};
    uint64_t engine_epoch{0};
    uint64_t evidence_id{0};

    bool operator==(const P6ToP4AudioEvent&) const = default;
};

}  // namespace kivo::video::audio_plane::boundary
