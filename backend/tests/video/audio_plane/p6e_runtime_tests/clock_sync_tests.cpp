// clock_sync_tests.cpp — P6E tests: clock + A/V sync
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/runtime/clock/audio_clock.hpp"
#include "video/audio_plane/runtime/sync/av_sync_controller.hpp"

namespace clk = kivo::video::audio_plane::backend::runtime::clock;
namespace syn = kivo::video::audio_plane::backend::runtime::sync;

static int g_failed = 0;

#define P6E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// E5: audio clock position
static void test_e05_clock_position() {
    clk::AudioClock c;
    c.set_sample_rate(48000);
    c.add_written(48000);  // 1 second
    c.update_played(24000);  // 0.5 second
    if (c.position_ms() != 500) throw std::runtime_error("position must be 500ms");
    if (c.buffered_samples() != 24000) throw std::runtime_error("buffered must be 24000");
    std::printf("[E05] clock position PASS\n");
}

// E6: A/V sync
static void test_e06_av_sync() {
    syn::AvSyncController s;
    s.set_audio_position_ms(1000);
    s.set_video_position_ms(1000);
    if (!s.is_in_sync(10)) throw std::runtime_error("must be in sync");
    if (s.sync_offset_ms() != 0) throw std::runtime_error("offset must be 0");

    s.set_audio_position_ms(1050);
    if (s.is_in_sync(10)) throw std::runtime_error("must NOT be in sync (50ms offset)");
    if (s.sync_offset_ms() != 50) throw std::runtime_error("offset must be 50");
    std::printf("[E06] A/V sync PASS\n");
}

int main() {
    std::printf("=== P6E Clock+Sync Tests ===\n\n");
    P6E_RUN(test_e05_clock_position);
    P6E_RUN(test_e06_av_sync);
    if (g_failed == 0) { std::printf("\n=== P6E Clock+Sync: ALL PASS ===\n"); return 0; }
    else { std::printf("\n=== P6E Clock+Sync: %d FAILED ===\n", g_failed); return 1; }
}
