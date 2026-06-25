#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#endif

#include <cstdint>
#include <string>
#include <atomic>

namespace kivo::cinema_engine {

// Internal WASAPI writer state, hidden behind opaque handle.
struct WasapiWriterImpl {
#ifdef _WIN32
    IMMDeviceEnumerator* enumerator{nullptr};
    IMMDevice* device{nullptr};
    IAudioClient* audio_client{nullptr};
    IAudioRenderClient* render_client{nullptr};
    WAVEFORMATEX* mix_format{nullptr};
    UINT32 buffer_frame_count{0};
    bool com_initialized{false};
    bool is_playing{false};
#endif
    int32_t sample_rate{0};
    int32_t channels{0};
    int32_t bits_per_sample{0};
    int64_t total_bytes_written{0};
    int64_t total_samples_written{0};
};

}  // namespace kivo::cinema_engine
