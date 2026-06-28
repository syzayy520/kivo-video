// audio_render_fill_loop_policy.hpp — P6A §17: fill loop + underfill
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::render {
struct AudioRenderFillLoopPolicy {
    ContractMetadata metadata;
    bool query_available_frames_once{true};
    bool getbuffer_releasebuffer_alternate{true};
    bool releasebuffer_count_matches_getbuffer{true};
    bool no_second_getbuffer_before_releasebuffer{true};
    bool silence_fill_recorded_as_evidence{true};
    bool deadlock_guard_decode_pause_canceled{true};
    bool deadlock_guard_queue_full_and_underfill_no_both_blocking{true};
};
}  // namespace kivo::video::audio_plane::render
