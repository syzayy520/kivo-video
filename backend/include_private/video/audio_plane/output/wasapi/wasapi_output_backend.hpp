// wasapi_output_backend.hpp — P6D backend: WASAPI audio output backend (private interface)
// Private interface: WASAPI types ARE allowed here but NOT in public.
#pragma once

#include <cstdint>
#include <memory>

#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/output/audio_output_backend_lifecycle.hpp"
#include "kivo/video/audio_plane/output/audio_output_open_request.hpp"
#include "kivo/video/audio_plane/output/audio_output_open_result.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputBackendState;
using kivo::video::audio_plane::output::AudioOutputOpenRequest;
using kivo::video::audio_plane::output::AudioOutputOpenResult;

enum class OutputInitError : int32_t {
    ComInitFailed = 0,
    NoDefaultDevice = 1,
    ClientActivateFailed = 2,
    ClientInitializeFailed = 3,
    RenderClientGetFailed = 4,
    InvalidState = 5,
};

enum class OutputStartError : int32_t {
    NotReady = 0,
    AlreadyRunning = 1,
    StartFailed = 2,
};

enum class OutputStopError : int32_t {
    NotRunning = 0,
    StopFailed = 1,
};

enum class OutputTransitionError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
};

// WasapiOutputBackend
// Real audio output backend using WASAPI (shared mode).
// State machine uses P6A AudioOutputBackendState (12 states).
class WasapiOutputBackend {
public:
    WasapiOutputBackend();
    ~WasapiOutputBackend();

    WasapiOutputBackend(const WasapiOutputBackend&) = delete;
    WasapiOutputBackend& operator=(const WasapiOutputBackend&) = delete;
    WasapiOutputBackend(WasapiOutputBackend&&) noexcept;
    WasapiOutputBackend& operator=(WasapiOutputBackend&&) noexcept;

    // Initialize: NotCreated → Created → Opening → Ready
    kivo::Expected<AudioOutputOpenResult, OutputInitError> init(const AudioOutputOpenRequest& request);

    // Start: Ready → Running
    kivo::Expected<std::monostate, OutputStartError> start();

    // Stop: Running/Paused → Ready
    kivo::Expected<std::monostate, OutputStopError> stop();

    // Close: Any → Closed
    void close();

    // Transition to explicit target
    kivo::Expected<std::monostate, OutputTransitionError> transition_to(AudioOutputBackendState target);

    AudioOutputBackendState state() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi
