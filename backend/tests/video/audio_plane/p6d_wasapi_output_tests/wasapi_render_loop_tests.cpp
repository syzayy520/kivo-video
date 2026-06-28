// wasapi_render_loop_tests.cpp — P6D tests: render loop (D9-D12)
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/output/wasapi/wasapi_render_loop.hpp"

namespace rt = kivo::video::audio_plane::backend::output::wasapi;
using kivo::video::audio_plane::backend::output::wasapi::RenderLoopState;

static int g_failed = 0;

#define P6D_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// D9: render loop starts in Idle
static void test_d09_idle_state() {
    rt::WasapiRenderLoop loop;
    if (loop.state() != RenderLoopState::Idle)
        throw std::runtime_error("initial state must be Idle");
    std::printf("[D09] idle state PASS\n");
}

// D10: start with valid callback → Running
static void test_d10_start() {
    rt::WasapiRenderLoop loop;
    auto cb = [](uint8_t*, int) -> int { return 0; };
    if (!loop.start(cb)) throw std::runtime_error("start must succeed");
    if (loop.state() != RenderLoopState::Running)
        throw std::runtime_error("state must be Running");
    loop.stop();
    std::printf("[D10] start PASS\n");
}

// D11: start with null callback → fail
static void test_d11_start_null() {
    rt::WasapiRenderLoop loop;
    rt::WasapiRenderLoop::FillCallback empty;
    if (loop.start(empty)) throw std::runtime_error("start with null must fail");
    std::printf("[D11] start null callback PASS\n");
}

// D12: stop → Stopped
static void test_d12_stop() {
    rt::WasapiRenderLoop loop;
    auto cb = [](uint8_t*, int) -> int { return 0; };
    loop.start(cb);
    loop.stop();
    if (loop.state() != RenderLoopState::Stopped)
        throw std::runtime_error("state must be Stopped");
    std::printf("[D12] stop PASS\n");
}

int main() {
    std::printf("=== P6D Render Loop Tests ===\n\n");
    P6D_RUN(test_d09_idle_state);
    P6D_RUN(test_d10_start);
    P6D_RUN(test_d11_start_null);
    P6D_RUN(test_d12_stop);
    if (g_failed == 0) {
        std::printf("\n=== P6D Render Loop: ALL PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6D Render Loop: %d FAILED ===\n", g_failed);
        return 1;
    }
}
