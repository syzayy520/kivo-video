#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// WriteResult contains the result of a PCM write operation.
struct WriteResult {
    bool success{false};
    int32_t samples_written{0};
    int32_t frames_written{0};
    bool is_buffer_full{false};
    int32_t available_frames{0};  // frames available in buffer after write
    std::string error_message;
};

// BufferStats contains statistics about the WASAPI buffer.
struct BufferStats {
    int32_t buffer_capacity_frames{0};
    int32_t buffer_available_frames{0};
    int32_t buffer_pending_frames{0};
    int64_t total_samples_written{0};
    int64_t total_bytes_written{0};
    double buffer_fill_percent{0.0};
};

// WasapiSharedPcmWriter writes PCM data to WASAPI shared mode buffer.
// This interface is platform-neutral; actual WASAPI calls stay inside adapter boundary.
struct WasapiSharedPcmWriter {
    ContractMetadata metadata;

    // Initialize the WASAPI shared mode writer with the given audio endpoint.
    bool initialize(const AudioEndpointRuntimeContract& endpoint);

    // Write PCM data to the WASAPI buffer.
    WriteResult write(const uint8_t* pcm_data, int32_t byte_size);

    // Get current buffer status.
    int32_t available_frames() const;

    // Get the last error message (empty if no error).
    std::string last_error() const;

    // Check if writer is initialized and ready.
    bool is_ready() const;

    // Start playback (if not already started).
    bool start();

    // Stop playback (if playing).
    bool stop();

    // Release all resources.
    void release();

private:
    AudioEndpointRuntimeContract endpoint_;
    bool initialized_{false};
    bool playing_{false};
    std::string last_error_;
    int64_t total_bytes_written_{0};
    int64_t total_samples_written_{0};
};

}  // namespace kivo::cinema_engine
