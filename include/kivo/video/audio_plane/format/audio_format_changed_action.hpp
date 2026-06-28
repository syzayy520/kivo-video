// audio_format_changed_action.hpp — P6A format: audio format changed action contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioFormatChangedAction {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool create_pending_pipeline_generation{true};
    bool stop_old_decode_publication{true};
    bool inject_format_barrier{true};
    bool discard_stale_with_budget{true};
    bool rebuild_converters{true};
    bool activate_render_generation{true};
    bool recompute_claim_generation{true};
    bool emit_evidence{true};
};
}  // namespace kivo::video::audio_plane::format
