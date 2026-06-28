// wasapi_output_backend.cpp — P6D backend: WasapiOutputBackend implementation
#include "video/audio_plane/output/wasapi/wasapi_output_backend.hpp"

#include <new>
#include <cstring>

#include "video/audio_plane/output/wasapi/wasapi_output_lifecycle.hpp"
#include "video/audio_plane/output/wasapi/wasapi_output_evidence_emitter.hpp"
#include "wasapi_com_initializer_guard.hpp"
#include "wasapi_device_enumerator_guard.hpp"
#include "wasapi_device_guard.hpp"
#include "wasapi_audio_client_guard.hpp"
#include "wasapi_render_client_guard.hpp"
#include "wasapi_format_helper.hpp"

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputBackendState;
using kivo::video::audio_plane::output::AudioOutputOpenRequest;
using kivo::video::audio_plane::output::AudioOutputOpenResult;
using kivo::video::audio_plane::output::AudioOutputOpenStatus;
using kivo::video::audio_plane::output::AudioOutputMode;
using kivo::video::audio_plane::output::AudioOutputBackendLifecycle;

struct WasapiOutputBackend::Impl {
    internal::ComInitializerGuard com_guard;
    internal::DeviceEnumeratorGuard enumerator_guard;
    internal::DeviceGuard device_guard;
    internal::AudioClientGuard client_guard;
    internal::RenderClientGuard render_guard;
    WasapiOutputLifecycleController lifecycle;
    WasapiOutputEvidenceEmitter evidence_emitter;
    AudioOutputBackendState state{AudioOutputBackendState::NotCreated};
    int32_t sample_rate{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
};

WasapiOutputBackend::WasapiOutputBackend()
    : impl_(std::make_unique<Impl>()) {}

WasapiOutputBackend::~WasapiOutputBackend() { close(); }

WasapiOutputBackend::WasapiOutputBackend(WasapiOutputBackend&&) noexcept = default;
WasapiOutputBackend& WasapiOutputBackend::operator=(WasapiOutputBackend&&) noexcept = default;

kivo::Expected<AudioOutputOpenResult, OutputInitError> WasapiOutputBackend::init(
    const AudioOutputOpenRequest& request) {
    if (impl_->state != AudioOutputBackendState::NotCreated) {
        return OutputInitError::InvalidState;
    }

    // COM init
    if (!impl_->com_guard.is_valid()) {
        return OutputInitError::ComInitFailed;
    }

    // Create device enumerator
    if (FAILED(impl_->enumerator_guard.initialize())) {
        return OutputInitError::ComInitFailed;
    }

    // Get default render endpoint
    IMMDevice* device = nullptr;
    if (FAILED(impl_->enumerator_guard.get()->GetDefaultAudioEndpoint(
            eRender, eConsole, &device)) || !device) {
        return OutputInitError::NoDefaultDevice;
    }
    impl_->device_guard.take(device);

    // Activate IAudioClient
    IAudioClient* client = nullptr;
    if (FAILED(device->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
            reinterpret_cast<void**>(&client))) || !client) {
        return OutputInitError::ClientActivateFailed;
    }
    impl_->client_guard.take(client);

    // Build format
    WAVEFORMATEX wfx = internal::make_pcm_format(
        request.sample_rate_hz, request.channel_count, request.bit_depth);

    // Initialize audio client (shared mode, no event callback yet — render loop sets it later)
    DWORD flags = 0;
    REFERENCE_TIME buffer_duration = static_cast<REFERENCE_TIME>(request.buffer_period_ms) * 10000;
    if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags,
            buffer_duration, 0, &wfx, nullptr))) {
        return OutputInitError::ClientInitializeFailed;
    }

    // Get render client
    IAudioRenderClient* render_client = nullptr;
    if (FAILED(client->GetService(
            __uuidof(IAudioRenderClient),
            reinterpret_cast<void**>(&render_client))) || !render_client) {
        return OutputInitError::RenderClientGetFailed;
    }
    impl_->render_guard.take(render_client);

    impl_->sample_rate = request.sample_rate_hz;
    impl_->channel_count = request.channel_count;
    impl_->bit_depth = request.bit_depth;

    // Pre-fill the buffer with silence before Start().
    // WASAPI requires at least one GetBuffer/ReleaseBuffer cycle before Start
    // to avoid AUDCLNT_E_NOT_STOPPED or silent-output issues.
    UINT32 buffer_frames = 0;
    if (SUCCEEDED(client->GetBufferSize(&buffer_frames)) && buffer_frames > 0) {
        UINT32 padding = 0;
        client->GetCurrentPadding(&padding);
        UINT32 frames_to_write = buffer_frames - padding;
        if (frames_to_write > 0) {
            BYTE* data = nullptr;
            if (SUCCEEDED(render_client->GetBuffer(frames_to_write, &data)) && data) {
                int bytes_per_frame = request.channel_count * request.bit_depth / 8;
                std::memset(data, 0, static_cast<size_t>(frames_to_write) * bytes_per_frame);
                render_client->ReleaseBuffer(frames_to_write, 0);
            }
        }
    }

    impl_->state = AudioOutputBackendState::Ready;

    AudioOutputOpenResult result;
    result.status = AudioOutputOpenStatus::Ok;
    result.actual_mode = AudioOutputMode::PcmSharedCompatible;
    result.actual_sample_rate_hz = request.sample_rate_hz;
    result.actual_channel_count = request.channel_count;
    result.actual_bit_depth = request.bit_depth;
    result.actual_buffer_period_ms = request.buffer_period_ms;
    return result;
}

kivo::Expected<std::monostate, OutputStartError> WasapiOutputBackend::start() {
    if (impl_->state != AudioOutputBackendState::Ready) {
        if (impl_->state == AudioOutputBackendState::Running)
            return OutputStartError::AlreadyRunning;
        return OutputStartError::NotReady;
    }
    if (FAILED(impl_->client_guard.get()->Start())) {
        return OutputStartError::StartFailed;
    }
    impl_->state = AudioOutputBackendState::Running;
    return std::monostate{};
}

kivo::Expected<std::monostate, OutputStopError> WasapiOutputBackend::stop() {
    if (impl_->state != AudioOutputBackendState::Running &&
        impl_->state != AudioOutputBackendState::Paused) {
        return OutputStopError::NotRunning;
    }
    if (FAILED(impl_->client_guard.get()->Stop())) {
        return OutputStopError::StopFailed;
    }
    impl_->state = AudioOutputBackendState::Ready;
    return std::monostate{};
}

void WasapiOutputBackend::close() {
    if (impl_->state == AudioOutputBackendState::Closed) return;
    if (impl_->client_guard.is_valid()) {
        impl_->client_guard.get()->Stop();
    }
    impl_->render_guard.reset();
    impl_->client_guard.reset();
    impl_->device_guard.reset();
    impl_->state = AudioOutputBackendState::Closed;
}

kivo::Expected<std::monostate, OutputTransitionError> WasapiOutputBackend::transition_to(
    AudioOutputBackendState target) {
    AudioOutputBackendLifecycle lifecycle;
    lifecycle.state = impl_->state;
    if (lifecycle.is_terminal()) {
        return OutputTransitionError::SourceIsTerminal;
    }
    auto t = impl_->lifecycle.transition(impl_->state, target);
    if (t.is_error()) {
        return OutputTransitionError::InvalidAdjacency;
    }
    impl_->state = target;
    return std::monostate{};
}

AudioOutputBackendState WasapiOutputBackend::state() const noexcept {
    return impl_->state;
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi
