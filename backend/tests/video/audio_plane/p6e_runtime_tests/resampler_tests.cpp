// resampler_tests.cpp — P6E tests: linear resampler
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "video/audio_plane/runtime/resampler/linear_resampler.hpp"

namespace rt = kivo::video::audio_plane::backend::runtime::resampler;

static int g_failed = 0;

#define P6E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// E1: resampler 48k→44.1k produces output
static void test_e01_resample_48k_to_44k() {
    rt::LinearResampler r;
    r.configure(48000, 44100, 2);
    if (r.src_rate() != 48000) throw std::runtime_error("src_rate must be 48000");
    if (r.dst_rate() != 44100) throw std::runtime_error("dst_rate must be 44100");

    std::vector<int16_t> input(480 * 2, 1000);  // 480 frames stereo
    std::vector<int16_t> output(1000 * 2, 0);
    int out_frames = r.process(input.data(), 480, output.data(), 1000);
    if (out_frames <= 0) throw std::runtime_error("must produce output frames");
    if (out_frames > 1000) throw std::runtime_error("output exceeds max");
    std::printf("[E01] resample 48k->44.1k: %d frames PASS\n", out_frames);
}

// E2: resampler same rate = passthrough (approximately)
static void test_e02_same_rate() {
    rt::LinearResampler r;
    r.configure(48000, 48000, 1);
    std::vector<int16_t> input(100, 500);
    std::vector<int16_t> output(200, 0);
    int out = r.process(input.data(), 100, output.data(), 200);
    if (out <= 0) throw std::runtime_error("same rate must produce output");
    std::printf("[E02] same rate: %d frames PASS\n", out);
}

int main() {
    std::printf("=== P6E Resampler Tests ===\n\n");
    P6E_RUN(test_e01_resample_48k_to_44k);
    P6E_RUN(test_e02_same_rate);
    if (g_failed == 0) { std::printf("\n=== P6E Resampler: ALL PASS ===\n"); return 0; }
    else { std::printf("\n=== P6E Resampler: %d FAILED ===\n", g_failed); return 1; }
}
