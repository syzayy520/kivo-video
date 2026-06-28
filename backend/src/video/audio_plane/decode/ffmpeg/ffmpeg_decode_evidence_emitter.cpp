// ffmpeg_decode_evidence_emitter.cpp — P6C backend: decode evidence emitter implementation
// Uses ONLY P6A DecodeEvidenceKind enum values — NO new kinds invented.
#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.hpp"

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeEvidence;
using kivo::video::audio_plane::decode::DecodeEvidenceKind;

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::make(
    DecodeEvidenceKind kind, uint64_t correlation_id) noexcept {
    AudioDecodeEvidence ev{};
    ev.kind = kind;
    ev.evidence_id = evidence_counter_++;
    ev.correlation_id = correlation_id;
    return ev;
}

uint64_t FFmpegDecodeEvidenceEmitter::next_evidence_id() noexcept {
    return evidence_counter_++;
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_init_ok(
    int32_t codec_id, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::InitOk, static_cast<uint64_t>(codec_id));
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_init_failed(
    int32_t codec_id, int32_t error_code, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::InitFailed,
                (static_cast<uint64_t>(codec_id) << 32) | static_cast<uint32_t>(error_code));
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_decode_ok(
    uint64_t frame_id, int64_t pts, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::DecodeOk, frame_id);
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_decode_failed(
    uint64_t packet_id, int32_t error_code, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::DecodeFailed,
                (packet_id << 32) | static_cast<uint32_t>(error_code));
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_timed_out(
    uint32_t deadline_ms, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::TimedOut, deadline_ms);
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_flushed(uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::Flushed, timestamp_ms);
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_drained(
    uint32_t frame_count, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::Drained, frame_count);
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_format_changed(uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::FormatChanged, timestamp_ms);
}

AudioDecodeEvidence FFmpegDecodeEvidenceEmitter::emit_superseded(
    uint64_t old_gen, uint64_t new_gen, uint64_t timestamp_ms) {
    return make(DecodeEvidenceKind::Superseded,
                (old_gen << 32) | static_cast<uint32_t>(new_gen));
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
