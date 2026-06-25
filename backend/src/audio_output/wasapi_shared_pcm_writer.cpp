#include "kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp"
#include "kivo/cinema_engine/audio_output/wasapi_internal.hpp"

#ifdef _WIN32
#include <iostream>
#include <thread>
#include <chrono>

namespace kivo::cinema_engine {

WasapiSharedPcmWriter::WasapiSharedPcmWriter() : impl_(nullptr) {}

WasapiSharedPcmWriter::~WasapiSharedPcmWriter() {
    release();
}

bool WasapiSharedPcmWriter::initialize(const AudioEndpointRuntimeContract& endpoint) {
    if (initialized_) {
        last_error_ = "Already initialized";
        return false;
    }

    endpoint_ = endpoint;

    // Create impl structure
    impl_ = new WasapiWriterImpl();

#ifdef _WIN32
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        last_error_ = "CoInitializeEx failed: " + std::to_string(hr);
        delete impl_;
        impl_ = nullptr;
        return false;
    }
    impl_->com_initialized = true;

    // Get device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&impl_->enumerator)
    );
    if (FAILED(hr)) {
        last_error_ = "CoCreateInstance(MMDeviceEnumerator) failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Get default audio endpoint
    hr = impl_->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &impl_->device);
    if (FAILED(hr)) {
        last_error_ = "GetDefaultAudioEndpoint failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Activate audio client
    hr = impl_->device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&impl_->audio_client)
    );
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::Activate failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Get mix format
    hr = impl_->audio_client->GetMixFormat(&impl_->mix_format);
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::GetMixFormat failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Initialize audio client in shared mode
    REFERENCE_TIME hnsPeriod = 0;
    hr = impl_->audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,  // stream flags
        hnsPeriod,  // buffer duration (0 = default)
        0,  // periodicity
        impl_->mix_format,
        nullptr
    );
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::Initialize failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Get buffer size
    hr = impl_->audio_client->GetBufferSize(&impl_->buffer_frame_count);
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::GetBufferSize failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Get render client
    hr = impl_->audio_client->GetService(
        __uuidof(IAudioRenderClient),
        reinterpret_cast<void**>(&impl_->render_client)
    );
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::GetService(IAudioRenderClient) failed: " + std::to_string(hr);
        release();
        return false;
    }

    // Store format info
    impl_->sample_rate = impl_->mix_format->nSamplesPerSec;
    impl_->channels = impl_->mix_format->nChannels;
    impl_->bits_per_sample = impl_->mix_format->wBitsPerSample;

    // Update endpoint info with actual format
    endpoint_.sample_rate = impl_->sample_rate;
    endpoint_.channels = impl_->channels;
    endpoint_.bit_depth = impl_->bits_per_sample;

    initialized_ = true;
    return true;
#else
    // Non-Windows: not supported
    last_error_ = "WASAPI not supported on this platform";
    return false;
#endif
}

WriteResult WasapiSharedPcmWriter::write(const uint8_t* pcm_data, int32_t byte_size) {
    WriteResult result;
    
    if (!initialized_ || !impl_) {
        result.success = false;
        result.error_message = "Not initialized";
        return result;
    }

#ifdef _WIN32
    // Get current padding
    UINT32 padding_frames = 0;
    HRESULT hr = impl_->audio_client->GetCurrentPadding(&padding_frames);
    if (FAILED(hr)) {
        result.success = false;
        result.error_message = "GetCurrentPadding failed: " + std::to_string(hr);
        return result;
    }

    // Calculate available frames
    UINT32 available_frames = impl_->buffer_frame_count - padding_frames;
    if (available_frames == 0) {
        result.success = false;
        result.error_message = "Buffer full";
        result.is_buffer_full = true;
        result.available_frames = 0;
        return result;
    }

    // Calculate frames to write
    int bytes_per_frame = impl_->channels * (impl_->bits_per_sample / 8);
    if (bytes_per_frame == 0) {
        result.success = false;
        result.error_message = "Invalid format";
        return result;
    }

    int32_t frames_to_write = byte_size / bytes_per_frame;
    if (frames_to_write <= 0) {
        result.success = false;
        result.error_message = "Invalid byte size";
        return result;
    }

    // Limit to available frames
    if (static_cast<UINT32>(frames_to_write) > available_frames) {
        frames_to_write = static_cast<int32_t>(available_frames);
    }

    // Get buffer
    BYTE* buffer = nullptr;
    hr = impl_->render_client->GetBuffer(frames_to_write, &buffer);
    if (FAILED(hr)) {
        result.success = false;
        result.error_message = "GetBuffer failed: " + std::to_string(hr);
        return result;
    }

    // Copy data
    int32_t bytes_to_copy = frames_to_write * bytes_per_frame;
    memcpy(buffer, pcm_data, bytes_to_copy);

    // Release buffer
    hr = impl_->render_client->ReleaseBuffer(frames_to_write, 0);
    if (FAILED(hr)) {
        result.success = false;
        result.error_message = "ReleaseBuffer failed: " + std::to_string(hr);
        return result;
    }

    // Update statistics
    result.success = true;
    result.frames_written = frames_to_write;
    result.samples_written = frames_to_write * impl_->channels;
    result.available_frames = static_cast<int32_t>(available_frames - frames_to_write);
    result.is_buffer_full = (result.available_frames == 0);

    impl_->total_bytes_written += bytes_to_copy;
    impl_->total_samples_written += result.samples_written;
    total_bytes_written_ += bytes_to_copy;
    total_samples_written_ += result.samples_written;

    return result;
#else
    result.success = false;
    result.error_message = "WASAPI not supported on this platform";
    return result;
#endif
}

int32_t WasapiSharedPcmWriter::available_frames() const {
    if (!initialized_ || !impl_) return 0;

#ifdef _WIN32
    UINT32 padding_frames = 0;
    HRESULT hr = impl_->audio_client->GetCurrentPadding(&padding_frames);
    if (FAILED(hr)) return 0;
    return static_cast<int32_t>(impl_->buffer_frame_count - padding_frames);
#else
    return 0;
#endif
}

std::string WasapiSharedPcmWriter::last_error() const { return last_error_; }

bool WasapiSharedPcmWriter::is_ready() const { return initialized_; }

bool WasapiSharedPcmWriter::start() {
    if (!initialized_ || !impl_) {
        last_error_ = "Not initialized";
        return false;
    }

#ifdef _WIN32
    if (impl_->is_playing) return true;  // already playing

    HRESULT hr = impl_->audio_client->Start();
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::Start failed: " + std::to_string(hr);
        return false;
    }
    impl_->is_playing = true;
    playing_ = true;
    return true;
#else
    last_error_ = "WASAPI not supported on this platform";
    return false;
#endif
}

bool WasapiSharedPcmWriter::stop() {
    if (!initialized_ || !impl_) {
        last_error_ = "Not initialized";
        return false;
    }

#ifdef _WIN32
    if (!impl_->is_playing) return true;  // already stopped

    HRESULT hr = impl_->audio_client->Stop();
    if (FAILED(hr)) {
        last_error_ = "IAudioClient::Stop failed: " + std::to_string(hr);
        return false;
    }
    impl_->is_playing = false;
    playing_ = false;
    return true;
#else
    last_error_ = "WASAPI not supported on this platform";
    return false;
#endif
}

void WasapiSharedPcmWriter::release() {
    if (impl_) {
#ifdef _WIN32
        // Stop if playing
        if (impl_->is_playing && impl_->audio_client) {
            impl_->audio_client->Stop();
        }

        // Release COM objects in reverse order
        if (impl_->render_client) {
            impl_->render_client->Release();
            impl_->render_client = nullptr;
        }
        if (impl_->mix_format) {
            CoTaskMemFree(impl_->mix_format);
            impl_->mix_format = nullptr;
        }
        if (impl_->audio_client) {
            impl_->audio_client->Release();
            impl_->audio_client = nullptr;
        }
        if (impl_->device) {
            impl_->device->Release();
            impl_->device = nullptr;
        }
        if (impl_->enumerator) {
            impl_->enumerator->Release();
            impl_->enumerator = nullptr;
        }
        if (impl_->com_initialized) {
            CoUninitialize();
        }
#endif
        delete impl_;
        impl_ = nullptr;
    }

    initialized_ = false;
    playing_ = false;
    last_error_.clear();
    total_bytes_written_ = 0;
    total_samples_written_ = 0;
}

}  // namespace kivo::cinema_engine

#else  // !_WIN32

// Non-Windows implementation: not supported
namespace kivo::cinema_engine {

WasapiSharedPcmWriter::WasapiSharedPcmWriter() : impl_(nullptr) {}
WasapiSharedPcmWriter::~WasapiSharedPcmWriter() { release(); }

bool WasapiSharedPcmWriter::initialize(const AudioEndpointRuntimeContract& endpoint) {
    endpoint_ = endpoint;
    last_error_ = "WASAPI not supported on this platform";
    return false;
}

WriteResult WasapiSharedPcmWriter::write(const uint8_t* pcm_data, int32_t byte_size) {
    WriteResult result;
    result.success = false;
    result.error_message = "WASAPI not supported on this platform";
    return result;
}

int32_t WasapiSharedPcmWriter::available_frames() const { return 0; }
std::string WasapiSharedPcmWriter::last_error() const { return last_error_; }
bool WasapiSharedPcmWriter::is_ready() const { return false; }
bool WasapiSharedPcmWriter::start() {
    last_error_ = "WASAPI not supported on this platform";
    return false;
}
bool WasapiSharedPcmWriter::stop() {
    last_error_ = "WASAPI not supported on this platform";
    return false;
}
void WasapiSharedPcmWriter::release() {
    initialized_ = false;
    playing_ = false;
    last_error_.clear();
}

}  // namespace kivo::cinema_engine

#endif  // _WIN32