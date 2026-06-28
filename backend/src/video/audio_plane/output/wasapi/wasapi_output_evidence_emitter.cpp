// wasapi_output_evidence_emitter.cpp — P6D backend: evidence emitter implementation
#include "video/audio_plane/output/wasapi/wasapi_output_evidence_emitter.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

using kivo::video::audio_plane::output::AudioOutputEvidence;
using kivo::video::audio_plane::output::OutputEvidenceKind;

AudioOutputEvidence WasapiOutputEvidenceEmitter::make(
    OutputEvidenceKind kind, uint64_t correlation_id) noexcept {
    AudioOutputEvidence ev{};
    ev.kind = kind;
    ev.evidence_id = counter_++;
    ev.correlation_id = correlation_id;
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_open_ok(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Opened, 0);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_open_failed(int32_t error_code, uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Opened, static_cast<uint64_t>(error_code));
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    ev.detail = error_code;
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_start_ok(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Opened, 1);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_start_failed(int32_t error_code, uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Opened, static_cast<uint64_t>(error_code));
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    ev.detail = error_code;
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_stop_ok(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Closed, 0);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_device_lost(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::DeviceLost, 0);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_recovered(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Recovered, 0);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_buffer_underrun(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Underfill, 0);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

AudioOutputEvidence WasapiOutputEvidenceEmitter::emit_closed(uint64_t timestamp_ms) {
    auto ev = make(OutputEvidenceKind::Closed, 1);
    ev.timestamp_ms = static_cast<int64_t>(timestamp_ms);
    return ev;
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi
