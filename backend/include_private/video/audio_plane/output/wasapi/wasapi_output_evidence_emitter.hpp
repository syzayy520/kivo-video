// wasapi_output_evidence_emitter.hpp — P6D backend: output evidence emitter
#pragma once

#include <cstdint>

#include "kivo/video/audio_plane/output/audio_output_evidence.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputEvidence;
using kivo::video::audio_plane::output::OutputEvidenceKind;

// WasapiOutputEvidenceEmitter
// Constructs AudioOutputEvidence records for WASAPI output lifecycle events.
class WasapiOutputEvidenceEmitter {
public:
    WasapiOutputEvidenceEmitter() = default;

    AudioOutputEvidence emit_open_ok(uint64_t timestamp_ms);
    AudioOutputEvidence emit_open_failed(int32_t error_code, uint64_t timestamp_ms);
    AudioOutputEvidence emit_start_ok(uint64_t timestamp_ms);
    AudioOutputEvidence emit_start_failed(int32_t error_code, uint64_t timestamp_ms);
    AudioOutputEvidence emit_stop_ok(uint64_t timestamp_ms);
    AudioOutputEvidence emit_device_lost(uint64_t timestamp_ms);
    AudioOutputEvidence emit_recovered(uint64_t timestamp_ms);
    AudioOutputEvidence emit_buffer_underrun(uint64_t timestamp_ms);
    AudioOutputEvidence emit_closed(uint64_t timestamp_ms);

private:
    AudioOutputEvidence make(OutputEvidenceKind kind, uint64_t correlation_id) noexcept;
    uint64_t counter_{1};
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi
