// wasapi_output_evidence_tests.cpp — P6D tests: evidence emitter (D13-D15)
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/output/wasapi/wasapi_output_evidence_emitter.hpp"
#include "kivo/video/audio_plane/output/audio_output_evidence.hpp"

namespace rt = kivo::video::audio_plane::backend::output::wasapi;
using kivo::video::audio_plane::output::AudioOutputEvidence;
using kivo::video::audio_plane::output::OutputEvidenceKind;

static int g_failed = 0;

#define P6D_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// D13: emit_open_ok → kind=Opened, evidence_id != 0
static void test_d13_emit_open_ok() {
    rt::WasapiOutputEvidenceEmitter emitter;
    auto ev = emitter.emit_open_ok(1000);
    if (ev.kind != OutputEvidenceKind::Opened) throw std::runtime_error("kind must be Opened");
    if (ev.evidence_id == 0) throw std::runtime_error("evidence_id must be non-zero");
    std::printf("[D13] emit_open_ok PASS\n");
}

// D14: emit_device_lost, emit_recovered, emit_buffer_underrun → correct kinds
static void test_d14_emit_lifecycle() {
    rt::WasapiOutputEvidenceEmitter emitter;
    auto ev_lost = emitter.emit_device_lost(2000);
    if (ev_lost.kind != OutputEvidenceKind::DeviceLost) throw std::runtime_error("must be DeviceLost");

    auto ev_rec = emitter.emit_recovered(3000);
    if (ev_rec.kind != OutputEvidenceKind::Recovered) throw std::runtime_error("must be Recovered");

    auto ev_underrun = emitter.emit_buffer_underrun(4000);
    if (ev_underrun.kind != OutputEvidenceKind::Underfill) throw std::runtime_error("must be Underfill");

    auto ev_closed = emitter.emit_closed(5000);
    if (ev_closed.kind != OutputEvidenceKind::Closed) throw std::runtime_error("must be Closed");

    std::printf("[D14] emit lifecycle PASS\n");
}

// D15: evidence IDs are monotonic
static void test_d15_monotonic_ids() {
    rt::WasapiOutputEvidenceEmitter emitter;
    auto ev1 = emitter.emit_open_ok(0);
    auto ev2 = emitter.emit_start_ok(0);
    auto ev3 = emitter.emit_stop_ok(0);
    if (ev2.evidence_id <= ev1.evidence_id) throw std::runtime_error("id must increase");
    if (ev3.evidence_id <= ev2.evidence_id) throw std::runtime_error("id must increase");
    std::printf("[D15] monotonic ids PASS\n");
}

int main() {
    std::printf("=== P6D Output Evidence Tests ===\n\n");
    P6D_RUN(test_d13_emit_open_ok);
    P6D_RUN(test_d14_emit_lifecycle);
    P6D_RUN(test_d15_monotonic_ids);
    if (g_failed == 0) {
        std::printf("\n=== P6D Evidence: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6D Evidence: %d FAILED ===\n", g_failed);
        return 1;
    }
}
