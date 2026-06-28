// ffmpeg_decode_evidence_tests.cpp — P6C tests: evidence emitter + state mapping (C13-C16)
// Tests P6A DecodeEvidenceKind usage and 16-state/9-kind model verification.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_evidence.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace rt = kivo::video::audio_plane::backend::decode::ffmpeg;
using kivo::video::audio_plane::decode::AudioDecodeEvidence;
using kivo::video::audio_plane::decode::DecodeEvidenceKind;
using kivo::video::audio_plane::decode::AudioDecodeBackendState;

static int g_failed = 0;

#define P6C_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// C13: emit_init_ok → kind=InitOk, evidence_id != 0
static void test_c13_emit_init_ok() {
    rt::FFmpegDecodeEvidenceEmitter emitter;
    auto ev = emitter.emit_init_ok(1, 1000);
    if (ev.kind != DecodeEvidenceKind::InitOk) throw std::runtime_error("kind must be InitOk");
    if (ev.evidence_id == 0) throw std::runtime_error("evidence_id must be non-zero");
    std::printf("[C13] emit_init_ok PASS\n");
}

// C14: emit_decode_ok → kind=DecodeOk
static void test_c14_emit_decode_ok() {
    rt::FFmpegDecodeEvidenceEmitter emitter;
    auto ev = emitter.emit_decode_ok(100, 48000, 2000);
    if (ev.kind != DecodeEvidenceKind::DecodeOk) throw std::runtime_error("kind must be DecodeOk");
    if (ev.evidence_id == 0) throw std::runtime_error("evidence_id must be non-zero");
    std::printf("[C14] emit_decode_ok PASS\n");
}

// C15: emit_timed_out, emit_flushed, emit_drained → correct kinds
static void test_c15_emit_lifecycle_evidence() {
    rt::FFmpegDecodeEvidenceEmitter emitter;
    auto ev_timeout = emitter.emit_timed_out(3000, 3000);
    if (ev_timeout.kind != DecodeEvidenceKind::TimedOut) throw std::runtime_error("kind must be TimedOut");

    auto ev_flushed = emitter.emit_flushed(4000);
    if (ev_flushed.kind != DecodeEvidenceKind::Flushed) throw std::runtime_error("kind must be Flushed");

    auto ev_drained = emitter.emit_drained(100, 5000);
    if (ev_drained.kind != DecodeEvidenceKind::Drained) throw std::runtime_error("kind must be Drained");

    auto ev_failed = emitter.emit_init_failed(1, 42, 6000);
    if (ev_failed.kind != DecodeEvidenceKind::InitFailed) throw std::runtime_error("kind must be InitFailed");

    auto ev_dec_fail = emitter.emit_decode_failed(1, 99, 7000);
    if (ev_dec_fail.kind != DecodeEvidenceKind::DecodeFailed) throw std::runtime_error("kind must be DecodeFailed");

    auto ev_fmt = emitter.emit_format_changed(8000);
    if (ev_fmt.kind != DecodeEvidenceKind::FormatChanged) throw std::runtime_error("kind must be FormatChanged");

    auto ev_sup = emitter.emit_superseded(1, 2, 9000);
    if (ev_sup.kind != DecodeEvidenceKind::Superseded) throw std::runtime_error("kind must be Superseded");

    std::printf("[C15] emit lifecycle evidence PASS\n");
}

// C16: State model mapping verification — all 16 states + 9 evidence kinds exist
static void test_c16_state_model_mapping() {
    // Verify all 16 AudioDecodeBackendState values exist and are distinct
    AudioDecodeBackendState states[] = {
        AudioDecodeBackendState::NotCreated,
        AudioDecodeBackendState::Created,
        AudioDecodeBackendState::WaitingCodecPrivateData,
        AudioDecodeBackendState::Opening,
        AudioDecodeBackendState::Ready,
        AudioDecodeBackendState::Decoding,
        AudioDecodeBackendState::Draining,
        AudioDecodeBackendState::Flushing,
        AudioDecodeBackendState::Resetting,
        AudioDecodeBackendState::FormatChanging,
        AudioDecodeBackendState::TimedOut,
        AudioDecodeBackendState::FailedRecoverable,
        AudioDecodeBackendState::FailedFatal,
        AudioDecodeBackendState::Closed,
        AudioDecodeBackendState::Superseded,
        AudioDecodeBackendState::FormatChanged,
    };
    constexpr int expected_count = 16;
    int distinct = 0;
    for (int i = 0; i < expected_count; ++i) {
        bool unique = true;
        for (int j = 0; j < i; ++j) {
            if (static_cast<int32_t>(states[i]) == static_cast<int32_t>(states[j])) {
                unique = false;
                break;
            }
        }
        if (unique) ++distinct;
    }
    if (distinct != expected_count)
        throw std::runtime_error("must have 16 distinct states");

    // Verify all 9 DecodeEvidenceKind values exist and are distinct
    DecodeEvidenceKind kinds[] = {
        DecodeEvidenceKind::InitOk,
        DecodeEvidenceKind::InitFailed,
        DecodeEvidenceKind::DecodeOk,
        DecodeEvidenceKind::DecodeFailed,
        DecodeEvidenceKind::TimedOut,
        DecodeEvidenceKind::Flushed,
        DecodeEvidenceKind::Drained,
        DecodeEvidenceKind::FormatChanged,
        DecodeEvidenceKind::Superseded,
    };
    constexpr int expected_kinds = 9;
    int distinct_kinds = 0;
    for (int i = 0; i < expected_kinds; ++i) {
        bool unique = true;
        for (int j = 0; j < i; ++j) {
            if (static_cast<int32_t>(kinds[i]) == static_cast<int32_t>(kinds[j])) {
                unique = false;
                break;
            }
        }
        if (unique) ++distinct_kinds;
    }
    if (distinct_kinds != expected_kinds)
        throw std::runtime_error("must have 9 distinct evidence kinds");

    // Verify no new kinds are invented (all emit_* return only P6A kinds)
    rt::FFmpegDecodeEvidenceEmitter emitter;
    auto all_evidence = {
        emitter.emit_init_ok(1, 0),
        emitter.emit_init_failed(1, 0, 0),
        emitter.emit_decode_ok(0, 0, 0),
        emitter.emit_decode_failed(0, 0, 0),
        emitter.emit_timed_out(0, 0),
        emitter.emit_flushed(0),
        emitter.emit_drained(0, 0),
        emitter.emit_format_changed(0),
        emitter.emit_superseded(0, 0, 0),
    };
    for (const auto& ev : all_evidence) {
        int32_t k = static_cast<int32_t>(ev.kind);
        if (k < 0 || k >= expected_kinds)
            throw std::runtime_error("evidence kind out of P6A range — new kind invented!");
    }

    std::printf("[C16] state model mapping (16 states + 9 kinds, no new invented) PASS\n");
}

int main() {
    std::printf("=== P6C Decode Evidence + State Mapping Tests ===\n\n");

    P6C_RUN(test_c13_emit_init_ok);
    P6C_RUN(test_c14_emit_decode_ok);
    P6C_RUN(test_c15_emit_lifecycle_evidence);
    P6C_RUN(test_c16_state_model_mapping);

    if (g_failed == 0) {
        std::printf("\n=== P6C Evidence: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6C Evidence: %d FAILED ===\n", g_failed);
        return 1;
    }
}
