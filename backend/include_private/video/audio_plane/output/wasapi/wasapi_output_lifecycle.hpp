// wasapi_output_lifecycle.hpp — P6D backend: WASAPI output lifecycle controller
#pragma once

#include <cstdint>

#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/output/audio_output_backend_lifecycle.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputBackendState;

enum class OutputLifecycleError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
};

// WasapiOutputLifecycleController
// Validates state transitions for WasapiOutputBackend.
class WasapiOutputLifecycleController {
public:
    WasapiOutputLifecycleController() = default;

    kivo::Expected<std::monostate, OutputLifecycleError> transition(
        AudioOutputBackendState from, AudioOutputBackendState to);

    static bool is_adjacent(AudioOutputBackendState from, AudioOutputBackendState to) noexcept;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi
