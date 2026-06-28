// wasapi_output_lifecycle.cpp — P6D backend: lifecycle controller implementation
#include "video/audio_plane/output/wasapi/wasapi_output_lifecycle.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputBackendState;
using kivo::video::audio_plane::output::AudioOutputBackendLifecycle;

bool WasapiOutputLifecycleController::is_adjacent(
    AudioOutputBackendState from, AudioOutputBackendState to) noexcept {
    // Universal: Any → Closed, Any → DeviceLost, Any → Failed
    if (to == AudioOutputBackendState::Closed ||
        to == AudioOutputBackendState::DeviceLost ||
        to == AudioOutputBackendState::Failed) {
        return true;
    }
    switch (from) {
        case AudioOutputBackendState::NotCreated:
            return to == AudioOutputBackendState::Created;
        case AudioOutputBackendState::Created:
            return to == AudioOutputBackendState::Opening;
        case AudioOutputBackendState::Opening:
            return to == AudioOutputBackendState::Ready || to == AudioOutputBackendState::Failed;
        case AudioOutputBackendState::Ready:
            return to == AudioOutputBackendState::Running || to == AudioOutputBackendState::Draining;
        case AudioOutputBackendState::Running:
            return to == AudioOutputBackendState::Paused ||
                   to == AudioOutputBackendState::Ready ||
                   to == AudioOutputBackendState::Draining ||
                   to == AudioOutputBackendState::Flushing;
        case AudioOutputBackendState::Paused:
            return to == AudioOutputBackendState::Running || to == AudioOutputBackendState::Ready;
        case AudioOutputBackendState::Draining:
            return to == AudioOutputBackendState::Ready;
        case AudioOutputBackendState::Flushing:
            return to == AudioOutputBackendState::Ready;
        case AudioOutputBackendState::Recovering:
            return to == AudioOutputBackendState::Ready || to == AudioOutputBackendState::Failed;
        case AudioOutputBackendState::DeviceLost:
            return to == AudioOutputBackendState::Recovering || to == AudioOutputBackendState::Closed;
        default:
            return false;
    }
}

kivo::Expected<std::monostate, OutputLifecycleError>
WasapiOutputLifecycleController::transition(
    AudioOutputBackendState from, AudioOutputBackendState to) {
    AudioOutputBackendLifecycle lifecycle;
    lifecycle.state = from;
    if (lifecycle.is_terminal()) {
        return OutputLifecycleError::SourceIsTerminal;
    }
    if (!is_adjacent(from, to)) {
        return OutputLifecycleError::InvalidAdjacency;
    }
    return std::monostate{};
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi
