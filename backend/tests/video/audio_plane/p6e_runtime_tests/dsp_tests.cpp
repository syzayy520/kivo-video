// dsp_tests.cpp — P6E tests: DSP processors
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/runtime/dsp/dither_processor.hpp"

namespace rt = kivo::video::audio_plane::backend::runtime::dsp;

static int g_failed = 0;

#define P6E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// E3: dither None = passthrough
static void test_e03_dither_none() {
    rt::DitherProcessor d;
    d.set_kind(rt::DitherKind::None);
    float result = d.process(0.5f);
    if (result != 0.5f) throw std::runtime_error("None must be passthrough");
    std::printf("[E03] dither None PASS\n");
}

// E4: dither Triangular adds noise
static void test_e04_dither_triangular() {
    rt::DitherProcessor d;
    d.set_kind(rt::DitherKind::Triangular);
    float r1 = d.process(0.0f);
    float r2 = d.process(0.0f);
    // Triangular dither should produce non-zero noise (with very high probability)
    if (r1 == 0.0f && r2 == 0.0f) throw std::runtime_error("triangular must add noise");
    std::printf("[E04] dither Triangular PASS\n");
}

int main() {
    std::printf("=== P6E DSP Tests ===\n\n");
    P6E_RUN(test_e03_dither_none);
    P6E_RUN(test_e04_dither_triangular);
    if (g_failed == 0) { std::printf("\n=== P6E DSP: ALL PASS ===\n"); return 0; }
    else { std::printf("\n=== P6E DSP: %d FAILED ===\n", g_failed); return 1; }
}
