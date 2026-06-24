#include "kivo/cinema_engine/audio_core/fake_audio_endpoint_service.hpp"

namespace kivo::cinema_engine {

AudioEndpointRuntimeContract build_shared_pcm_endpoint(const std::string& endpoint_id,
                                                      int32_t sample_rate,
                                                      int32_t channels,
                                                      int32_t bit_depth) {
    AudioEndpointRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-shared-pcm";
    contract.endpoint_id = endpoint_id;
    contract.endpoint_name = "Shared PCM Endpoint";
    contract.output_mode = "shared_pcm";
    contract.sample_rate = sample_rate;
    contract.channels = channels;
    contract.bit_depth = bit_depth;
    contract.is_initialized = true;
    contract.is_playing = false;
    contract.samples_written = 0;
    contract.buffer_duration_ms = 10.0;
    return contract;
}

AudioEndpointRuntimeContract build_uninitialized_endpoint(const std::string& endpoint_id) {
    AudioEndpointRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-uninitialized";
    contract.endpoint_id = endpoint_id;
    contract.endpoint_name = "Uninitialized Endpoint";
    contract.output_mode = "shared_pcm";
    contract.sample_rate = 0;
    contract.channels = 0;
    contract.bit_depth = 0;
    contract.is_initialized = false;
    contract.is_playing = false;
    contract.samples_written = 0;
    contract.buffer_duration_ms = 0.0;
    return contract;
}

AudioEndpointRuntimeContract build_playing_endpoint(const std::string& endpoint_id,
                                                   int64_t samples_written,
                                                   double buffer_duration_ms) {
    AudioEndpointRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-playing";
    contract.endpoint_id = endpoint_id;
    contract.endpoint_name = "Playing Endpoint";
    contract.output_mode = "shared_pcm";
    contract.sample_rate = 48000;
    contract.channels = 2;
    contract.bit_depth = 16;
    contract.is_initialized = true;
    contract.is_playing = true;
    contract.samples_written = samples_written;
    contract.buffer_duration_ms = buffer_duration_ms;
    return contract;
}

AudioEndpointRuntimeContract build_exclusive_pcm_endpoint(const std::string& endpoint_id,
                                                         int32_t sample_rate,
                                                         int32_t channels,
                                                         int32_t bit_depth) {
    AudioEndpointRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-exclusive-pcm";
    contract.endpoint_id = endpoint_id;
    contract.endpoint_name = "Exclusive PCM Endpoint";
    contract.output_mode = "exclusive_pcm";
    contract.sample_rate = sample_rate;
    contract.channels = channels;
    contract.bit_depth = bit_depth;
    contract.is_initialized = true;
    contract.is_playing = false;
    contract.samples_written = 0;
    contract.buffer_duration_ms = 10.0;
    return contract;
}

AudioEndpointRuntimeContract build_passthrough_endpoint(const std::string& endpoint_id,
                                                       int32_t sample_rate,
                                                       int32_t channels,
                                                       int32_t bit_depth) {
    AudioEndpointRuntimeContract contract;
    contract.metadata.schema_version = "1.1";
    contract.metadata.trace_id = "fake-passthrough";
    contract.endpoint_id = endpoint_id;
    contract.endpoint_name = "Passthrough Endpoint";
    contract.output_mode = "passthrough";
    contract.sample_rate = sample_rate;
    contract.channels = channels;
    contract.bit_depth = bit_depth;
    contract.is_initialized = true;
    contract.is_playing = false;
    contract.samples_written = 0;
    contract.buffer_duration_ms = 10.0;
    return contract;
}

}  // namespace kivo::cinema_engine