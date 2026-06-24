#pragma once

#include <string>

#include "kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

/// Build a default AudioEndpointRuntimeContract for shared PCM output.
AudioEndpointRuntimeContract build_shared_pcm_endpoint(const std::string& endpoint_id,
                                                      int32_t sample_rate = 48000,
                                                      int32_t channels = 2,
                                                      int32_t bit_depth = 16);

/// Build an uninitialized endpoint.
AudioEndpointRuntimeContract build_uninitialized_endpoint(const std::string& endpoint_id);

/// Build a playing endpoint with samples written.
AudioEndpointRuntimeContract build_playing_endpoint(const std::string& endpoint_id,
                                                   int64_t samples_written,
                                                   double buffer_duration_ms = 10.0);

/// Build an exclusive PCM endpoint.
AudioEndpointRuntimeContract build_exclusive_pcm_endpoint(const std::string& endpoint_id,
                                                         int32_t sample_rate = 48000,
                                                         int32_t channels = 2,
                                                         int32_t bit_depth = 24);

/// Build a passthrough endpoint (e.g., for encoded audio).
AudioEndpointRuntimeContract build_passthrough_endpoint(const std::string& endpoint_id,
                                                       int32_t sample_rate = 48000,
                                                       int32_t channels = 8,
                                                       int32_t bit_depth = 32);

}  // namespace kivo::cinema_engine