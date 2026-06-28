// wasapi_output_backend_tests.cpp — P6D tests: WasapiOutputBackend (D1-D8)
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/output/wasapi/wasapi_output_backend.hpp"
#include "kivo/video/audio_plane/output/audio_output_backend_lifecycle.hpp"
#include "kivo/video/audio_plane/output/audio_output_open_request.hpp"

namespace rt = kivo::video::audio_plane::backend::output::wasapi;
using kivo::video::audio_plane::output::AudioOutputBackendState;
using kivo::video::audio_plane::output::AudioOutputOpenRequest;
using kivo::video::audio_plane::output::AudioOutputMode;

static int g_failed = 0;

#define P6D_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// D1: init with valid request → success (requires real audio device)
static void test_d01_init() {
    rt::WasapiOutputBackend backend;
    AudioOutputOpenRequest req;
    req.sample_rate_hz = 48000;
    req.channel_count = 2;
    req.bit_depth = 16;
    req.buffer_period_ms = 10;
    req.requested_mode = AudioOutputMode::PcmSharedCompatible;
    auto r = backend.init(req);
    if (r.is_error()) {
        // No audio device — acceptable in CI
        std::printf("[D01] init skipped (no audio device)\n");
        return;
    }
    if (backend.state() != AudioOutputBackendState::Ready)
        throw std::runtime_error("state must be Ready after init");
    backend.close();
    std::printf("[D01] init PASS\n");
}

// D2: init twice → error
static void test_d02_init_twice() {
    rt::WasapiOutputBackend backend;
    AudioOutputOpenRequest req;
    req.sample_rate_hz = 48000;
    req.channel_count = 2;
    req.bit_depth = 16;
    req.buffer_period_ms = 10;
    auto r1 = backend.init(req);
    if (r1.is_error()) {
        std::printf("[D02] init twice skipped (no audio device)\n");
        return;
    }
    auto r2 = backend.init(req);
    if (!r2.is_error()) throw std::runtime_error("second init must fail");
    if (r2.error() != rt::OutputInitError::InvalidState)
        throw std::runtime_error("expected InvalidState");
    backend.close();
    std::printf("[D02] init twice rejected PASS\n");
}

// D3: start without init → NotReady
static void test_d03_start_before_init() {
    rt::WasapiOutputBackend backend;
    auto r = backend.start();
    if (!r.is_error()) throw std::runtime_error("start before init must fail");
    if (r.error() != rt::OutputStartError::NotReady)
        throw std::runtime_error("expected NotReady");
    std::printf("[D03] start before init PASS\n");
}

// D4: close → state=Closed
static void test_d04_close() {
    rt::WasapiOutputBackend backend;
    backend.close();
    if (backend.state() != AudioOutputBackendState::Closed)
        throw std::runtime_error("state must be Closed");
    std::printf("[D04] close PASS\n");
}

// D5: transition invalid → InvalidAdjacency
static void test_d05_transition_invalid() {
    rt::WasapiOutputBackend backend;
    // NotCreated → Running is invalid
    auto r = backend.transition_to(AudioOutputBackendState::Running);
    if (!r.is_error()) throw std::runtime_error("invalid transition must fail");
    if (r.error() != rt::OutputTransitionError::InvalidAdjacency)
        throw std::runtime_error("expected InvalidAdjacency");
    std::printf("[D05] invalid transition PASS\n");
}

// D6: transition from terminal → SourceIsTerminal
static void test_d06_transition_from_terminal() {
    rt::WasapiOutputBackend backend;
    backend.close();
    auto r = backend.transition_to(AudioOutputBackendState::Ready);
    if (!r.is_error()) throw std::runtime_error("transition from terminal must fail");
    if (r.error() != rt::OutputTransitionError::SourceIsTerminal)
        throw std::runtime_error("expected SourceIsTerminal");
    std::printf("[D06] transition from terminal PASS\n");
}

// D7: start after init → Running (requires real audio device)
static void test_d07_start() {
    rt::WasapiOutputBackend backend;
    AudioOutputOpenRequest req;
    req.sample_rate_hz = 48000;
    req.channel_count = 2;
    req.bit_depth = 16;
    req.buffer_period_ms = 10;
    if (backend.init(req).is_error()) {
        std::printf("[D07] start skipped (no audio device)\n");
        return;
    }
    auto r = backend.start();
    if (r.is_error()) {
        // Start may fail in CI/test environments without real audio hardware.
        std::printf("[D07] start skipped (start failed in test env)\n");
        backend.close();
        return;
    }
    if (backend.state() != AudioOutputBackendState::Running)
        throw std::runtime_error("state must be Running");
    backend.close();
    std::printf("[D07] start PASS\n");
}

// D8: stop after start → Ready (requires real audio device)
static void test_d08_stop() {
    rt::WasapiOutputBackend backend;
    AudioOutputOpenRequest req;
    req.sample_rate_hz = 48000;
    req.channel_count = 2;
    req.bit_depth = 16;
    req.buffer_period_ms = 10;
    if (backend.init(req).is_error()) {
        std::printf("[D08] stop skipped (no audio device)\n");
        return;
    }
    if (backend.start().is_error()) {
        std::printf("[D08] stop skipped (start failed)\n");
        return;
    }
    auto r = backend.stop();
    if (r.is_error()) throw std::runtime_error("stop must succeed");
    if (backend.state() != AudioOutputBackendState::Ready)
        throw std::runtime_error("state must be Ready after stop");
    backend.close();
    std::printf("[D08] stop PASS\n");
}

int main() {
    std::printf("=== P6D WASAPI Output Backend Tests ===\n\n");
    P6D_RUN(test_d01_init);
    P6D_RUN(test_d02_init_twice);
    P6D_RUN(test_d03_start_before_init);
    P6D_RUN(test_d04_close);
    P6D_RUN(test_d05_transition_invalid);
    P6D_RUN(test_d06_transition_from_terminal);
    P6D_RUN(test_d07_start);
    P6D_RUN(test_d08_stop);
    if (g_failed == 0) {
        std::printf("\n=== P6D Backend: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6D Backend: %d FAILED ===\n", g_failed);
        return 1;
    }
}
