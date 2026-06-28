// ffmpeg_decode_evidence_emitter.hpp — P6C backend: decode evidence emitter (private interface)
// Emits P6A AudioDecodeEvidence records. Uses ONLY DecodeEvidenceKind enum values (NO new kinds).
#pragma once

#include <cstdint>

#include "kivo/video/audio_plane/decode/audio_decode_evidence.hpp"

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeEvidence;
using kivo::video::audio_plane::decode::DecodeEvidenceKind;

// FFmpegDecodeEvidenceEmitter
// Constructs AudioDecodeEvidence records for FFmpeg decode lifecycle events.
// All emit_* methods return an AudioDecodeEvidence with the appropriate
// DecodeEvidenceKind (InitOk/InitFailed/DecodeOk/DecodeFailed/TimedOut/
// Flushed/Drained/FormatChanged/Superseded — NO new kinds invented).
// evidence_id is generated from an atomic counter starting at 1 (0 = invalid).
class FFmpegDecodeEvidenceEmitter {
public:
    FFmpegDecodeEvidenceEmitter() = default;

    AudioDecodeEvidence emit_init_ok(int32_t codec_id, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_init_failed(int32_t codec_id, int32_t error_code, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_decode_ok(uint64_t frame_id, int64_t pts, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_decode_failed(uint64_t packet_id, int32_t error_code, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_timed_out(uint32_t deadline_ms, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_flushed(uint64_t timestamp_ms);
    AudioDecodeEvidence emit_drained(uint32_t frame_count, uint64_t timestamp_ms);
    AudioDecodeEvidence emit_format_changed(uint64_t timestamp_ms);
    AudioDecodeEvidence emit_superseded(uint64_t old_gen, uint64_t new_gen, uint64_t timestamp_ms);

    uint64_t next_evidence_id() noexcept;

private:
    AudioDecodeEvidence make(DecodeEvidenceKind kind, uint64_t correlation_id) noexcept;

    // Counter starts at 1; 0 is reserved as "invalid" evidence_id.
    uint64_t evidence_counter_{1};
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
