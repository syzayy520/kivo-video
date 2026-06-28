// gap_tests.cpp — P6E tests: drift + quantize + noise shaping + passthrough + gapless + device thread
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "video/audio_plane/runtime/clock/drift_corrector.hpp"
#include "video/audio_plane/runtime/dsp/quantize_processor.hpp"
#include "video/audio_plane/runtime/dsp/noise_shaping_processor.hpp"
#include "video/audio_plane/runtime/passthrough/iec61937_parser.hpp"
#include "video/audio_plane/runtime/gapless/gapless_runtime.hpp"
#include "video/audio_plane/runtime/device_thread/device_thread_runtime.hpp"

namespace clk = kivo::video::audio_plane::backend::runtime::clock;
namespace dsp = kivo::video::audio_plane::backend::runtime::dsp;
namespace pas = kivo::video::audio_plane::backend::runtime::passthrough;
namespace gap = kivo::video::audio_plane::backend::runtime::gapless;
namespace dev = kivo::video::audio_plane::backend::runtime::device_thread;

static int g_failed = 0;

#define P6E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// G1: drift corrector — no drift within threshold
static void test_g01_drift_no_correction() {
    clk::DriftCorrector d;
    d.set_reference_ms(1000);
    d.set_audio_ms(1005);
    if (d.needs_correction(10)) throw std::runtime_error("5ms drift should not need correction");
    if (d.correction_ratio(10) != 1.0) throw std::runtime_error("ratio must be 1.0");
    std::printf("[G01] drift no correction PASS\n");
}

// G2: drift corrector — correction needed
static void test_g02_drift_correction() {
    clk::DriftCorrector d;
    d.set_reference_ms(1000);
    d.set_audio_ms(1100);  // 100ms ahead
    if (!d.needs_correction(10)) throw std::runtime_error("100ms drift must need correction");
    double ratio = d.correction_ratio(10);
    if (ratio >= 1.0) throw std::runtime_error("audio ahead → ratio must be < 1.0");
    std::printf("[G02] drift correction PASS (ratio=%.6f)\n", ratio);
}

// G3: quantize processor — 16-bit
static void test_g03_quantize_16bit() {
    dsp::QuantizeProcessor q;
    q.set_target_bit_depth(16);
    int32_t result = q.process(0.5f);
    // 0.5 * 32767 = 16383.5, round = 16384 (or 16383 due to float precision)
    if (result < 16380 || result > 16388)
        throw std::runtime_error("quantize 0.5 should be ~16384");
    // Clamp test
    int32_t max_val = q.process(2.0f);
    if (max_val != 32767) throw std::runtime_error("clamped max must be 32767");
    int32_t min_val = q.process(-2.0f);
    if (min_val != -32768) throw std::runtime_error("clamped min must be -32768");
    // Zero test
    int32_t zero = q.process(0.0f);
    if (zero != 0) throw std::runtime_error("quantize 0.0 must be 0");
    std::printf("[G03] quantize 16-bit PASS (result=%d)\n", result);
}

// G4: noise shaping — feedback accumulates error
static void test_g04_noise_shaping() {
    dsp::NoiseShapingProcessor ns;
    ns.reset();
    float err1 = ns.process(0.7f, 0.5f);  // error = (0.7-0.5)*0.5 = 0.1
    if (err1 < 0.05f || err1 > 0.15f)
        throw std::runtime_error("first error should be ~0.1");
    // Apply feedback: next sample = input + error
    float adjusted = ns.apply_feedback(0.7f);
    if (adjusted < 0.75f || adjusted > 0.85f)
        throw std::runtime_error("adjusted should be ~0.8");
    std::printf("[G04] noise shaping PASS\n");
}

// G5: IEC61937 parser — valid AC3 burst
static void test_g05_iec61937_ac3() {
    pas::Iec61937Parser parser;
    // Pa=0xF872, Pb=0x4E1F, Pc=0x0001 (AC3), Pd=1536 (burst size)
    uint16_t words[] = {0xF872, 0x4E1F, 0x0001, 1536};
    if (!parser.is_burst_start(words, 4)) throw std::runtime_error("must detect burst start");
    auto info = parser.parse_preamble(words, 4);
    if (!info.valid) throw std::runtime_error("preamble must be valid");
    if (info.type != pas::Iec61937BurstType::Ac3) throw std::runtime_error("type must be AC3");
    if (info.burst_size != 1536) throw std::runtime_error("burst size must be 1536");
    std::printf("[G05] IEC61937 AC3 PASS\n");
}

// G6: IEC61937 parser — invalid sync
static void test_g06_iec61937_invalid() {
    pas::Iec61937Parser parser;
    uint16_t words[] = {0x1234, 0x5678, 0x0001, 100};
    if (parser.is_burst_start(words, 4)) throw std::runtime_error("must not detect invalid sync");
    auto info = parser.parse_preamble(words, 4);
    if (info.valid) throw std::runtime_error("invalid preamble must not be valid");
    std::printf("[G06] IEC61937 invalid PASS\n");
}

// G7: gapless — codec delay + encoder padding trim
static void test_g07_gapless_trim() {
    gap::GaplessRuntime g;
    g.set_codec_delay_samples(576);      // MP3 delay
    g.set_encoder_padding_samples(1408);  // MP3 padding
    g.set_total_samples(100000);
    if (g.audible_start_sample() != 576) throw std::runtime_error("start must be 576");
    if (g.audible_end_sample() != 98592) throw std::runtime_error("end must be 98592");
    if (g.audible_sample_count() != 98016) throw std::runtime_error("count must be 98016");
    if (!g.is_audible(1000)) throw std::runtime_error("1000 must be audible");
    if (g.is_audible(100)) throw std::runtime_error("100 must not be audible (in delay)");
    if (g.is_audible(99000)) throw std::runtime_error("99000 must not be audible (in padding)");
    std::printf("[G07] gapless trim PASS\n");
}

// G8: device thread — start/stop state machine
static void test_g08_device_thread() {
    dev::DeviceThreadRuntime dt;
    if (dt.state() != dev::DeviceThreadState::NotStarted)
        throw std::runtime_error("initial state must be NotStarted");
    auto cb = []() {};
    if (!dt.start(cb)) throw std::runtime_error("start must succeed");
    if (dt.state() != dev::DeviceThreadState::Running)
        throw std::runtime_error("state must be Running");
    // Double start must fail
    if (dt.start(cb)) throw std::runtime_error("double start must fail");
    dt.stop();
    if (dt.state() != dev::DeviceThreadState::Stopped)
        throw std::runtime_error("state must be Stopped");
    std::printf("[G08] device thread PASS\n");
}

// G9: device thread — null callback must fail
static void test_g09_device_thread_null() {
    dev::DeviceThreadRuntime dt;
    dev::DeviceThreadRuntime::RenderCallback empty;
    if (dt.start(empty)) throw std::runtime_error("null callback must fail");
    std::printf("[G09] device thread null callback PASS\n");
}

int main() {
    std::printf("=== P6E Gap Tests ===\n\n");
    P6E_RUN(test_g01_drift_no_correction);
    P6E_RUN(test_g02_drift_correction);
    P6E_RUN(test_g03_quantize_16bit);
    P6E_RUN(test_g04_noise_shaping);
    P6E_RUN(test_g05_iec61937_ac3);
    P6E_RUN(test_g06_iec61937_invalid);
    P6E_RUN(test_g07_gapless_trim);
    P6E_RUN(test_g08_device_thread);
    P6E_RUN(test_g09_device_thread_null);
    if (g_failed == 0) { std::printf("\n=== P6E Gaps: ALL PASS ===\n"); return 0; }
    else { std::printf("\n=== P6E Gaps: %d FAILED ===\n", g_failed); return 1; }
}
