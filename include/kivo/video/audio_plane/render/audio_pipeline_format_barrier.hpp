// audio_pipeline_format_barrier.hpp — P6A §26: format barrier for FormatChanged
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/generation/audio_pipeline_generation.hpp"
namespace kivo::video::audio_plane::render {
struct AudioPipelineFormatBarrier {
    ContractMetadata metadata;
    generation::AudioPipelineGeneration pending_pipeline_generation;
    generation::AudioPipelineGeneration active_render_generation;
    bool prepare_thread_discards_old_generation{true};
    bool device_thread_discards_stale_with_budget{true};
    bool claim_fresh_only_after_active_output_matches{true};
};
}  // namespace kivo::video::audio_plane::render
