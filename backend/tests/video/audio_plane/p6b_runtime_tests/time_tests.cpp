// time_tests.cpp — P6B runtime time tests (B13-B14)
// Tests FakeCheckedRescale (B13) and FakePtsWrapDetector (B14).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "kivo/video/audio_plane/runtime/time/fake_checked_rescale.hpp"
#include "kivo/video/audio_plane/runtime/time/fake_pts_wrap_detector.hpp"
#include "kivo/video/audio_plane/time/audio_checked_rescale.hpp"

using namespace kivo::video::audio_plane::runtime::time;
using kivo::video::audio_plane::time::TimestampOverflow;

static int g_failed = 0;

#define P6B_RUN(name) \
    do { \
        try { name(); } \
        catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// B13: FakeCheckedRescale
// ---------------------------------------------------------------------------

// Valid rescale: pts=1000 from 1/1000 -> 1/90000 yields 90000.
static void test_b13_rescale_valid() {
    FakeCheckedRescale rescaler;
    // src = 1/1000 (1 kHz), dst = 1/90000 (90 kHz).
    auto r = rescaler.rescale(/*pts*/ 1000,
                              /*src_num*/ 1, /*src_den*/ 1000,
                              /*dst_num*/ 1, /*dst_den*/ 90000);
    if (!r.has_value()) throw std::runtime_error("valid rescale should not overflow");
    if (r.value() != 90000ll) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "expected 90000, got %lld", (long long)r.value());
        throw std::runtime_error(buf);
    }

    std::printf("[B13] rescale valid PASS\n");
}

// Another valid case: pts=1 from 1/1 -> 1/1000 yields 1000.
static void test_b13_rescale_valid_simple_upscale() {
    FakeCheckedRescale rescaler;
    auto r = rescaler.rescale(/*pts*/ 1,
                              /*src_num*/ 1, /*src_den*/ 1,
                              /*dst_num*/ 1, /*dst_den*/ 1000);
    if (!r.has_value()) throw std::runtime_error("simple upscale should not overflow");
    if (r.value() != 1000ll) throw std::runtime_error("expected 1000");

    std::printf("[B13] rescale valid simple upscale PASS\n");
}

// Overflow: pts=INT64_MAX with a large multiplier must report MultiplyOverflow.
static void test_b13_rescale_overflow() {
    FakeCheckedRescale rescaler;
    auto r = rescaler.rescale(/*pts*/ INT64_MAX,
                              /*src_num*/ 2, /*src_den*/ 1,
                              /*dst_num*/ 1, /*dst_den*/ 1);
    if (r.has_value()) throw std::runtime_error("overflow must not produce a value");
    if (r.error() != TimestampOverflow::MultiplyOverflow) {
        throw std::runtime_error("overflow must report MultiplyOverflow");
    }

    std::printf("[B13] rescale overflow PASS\n");
}

// Zero denominator in either source or destination yields an error.
static void test_b13_rescale_zero_denominator() {
    FakeCheckedRescale rescaler;

    // src_den == 0.
    auto r1 = rescaler.rescale(/*pts*/ 1000,
                               /*src_num*/ 1, /*src_den*/ 0,
                               /*dst_num*/ 1, /*dst_den*/ 90000);
    if (r1.has_value()) throw std::runtime_error("src_den=0 must not produce a value");
    if (r1.error() != TimestampOverflow::MultiplyOverflow) {
        throw std::runtime_error("src_den=0 must report MultiplyOverflow");
    }

    // dst_den == 0.
    auto r2 = rescaler.rescale(/*pts*/ 1000,
                               /*src_num*/ 1, /*src_den*/ 1000,
                               /*dst_num*/ 1, /*dst_den*/ 0);
    if (r2.has_value()) throw std::runtime_error("dst_den=0 must not produce a value");
    if (r2.error() != TimestampOverflow::MultiplyOverflow) {
        throw std::runtime_error("dst_den=0 must report MultiplyOverflow");
    }

    std::printf("[B13] rescale zero denominator PASS\n");
}

// src_num == 0 collapses the result to 0 (no time base at source).
static void test_b13_rescale_zero_src_num_yields_zero() {
    FakeCheckedRescale rescaler;
    auto r = rescaler.rescale(/*pts*/ 12345,
                              /*src_num*/ 0, /*src_den*/ 1000,
                              /*dst_num*/ 1, /*dst_den*/ 90000);
    if (!r.has_value()) throw std::runtime_error("src_num=0 should yield a value (0)");
    if (r.value() != 0ll) throw std::runtime_error("src_num=0 should yield 0");

    std::printf("[B13] rescale zero src_num yields zero PASS\n");
}

// ---------------------------------------------------------------------------
// B14: FakePtsWrapDetector
// ---------------------------------------------------------------------------

// NoWrap: prev=100, current=200 (normal forward progression within tolerance).
static void test_b14_no_wrap_forward_progression() {
    FakePtsWrapDetector det;
    // First sample seeds prev and returns NoWrap.
    auto first = det.detect(0, 100);
    if (first != WrapStatus::NoWrap) throw std::runtime_error("first sample must be NoWrap");

    auto second = det.detect(100, 200);
    if (second != WrapStatus::NoWrap) {
        throw std::runtime_error("forward 100->200 must be NoWrap");
    }

    std::printf("[B14] NoWrap forward progression PASS\n");
}

// ForwardWrap: prev near INT64_MAX, current small, large backward delta
// exceeding the wrap threshold.
static void test_b14_forward_wrap() {
    FakePtsWrapDetector det;
    // Lower the wrap threshold so the test is tractable. Use a small threshold
    // (e.g. 1<<20) so prev=INT64_MAX-100 -> current=50 clearly exceeds it.
    det.configure(/*wrap_threshold*/ (1ll << 20),
                  /*backward_tolerance*/ 1000,
                  /*max_forward_jump*/ 1000000);

    // Seed prev with a value near INT64_MAX.
    auto seed = det.detect(0, INT64_MAX - 100);
    if (seed != WrapStatus::NoWrap) throw std::runtime_error("seed must be NoWrap");

    // Now jump to a small value: backward delta is huge -> ForwardWrap.
    auto status = det.detect(INT64_MAX - 100, 50);
    if (status != WrapStatus::ForwardWrap) {
        throw std::runtime_error("expected ForwardWrap");
    }

    std::printf("[B14] ForwardWrap PASS\n");
}

// BackwardJump: prev=200, current=100, small delta within tolerance.
static void test_b14_backward_jump_small_delta() {
    FakePtsWrapDetector det;
    // Use a small tolerance so 200->100 is within it, and a large wrap
    // threshold so it is not classified as a wrap.
    det.configure(/*wrap_threshold*/ (1ll << 40),
                  /*backward_tolerance*/ 1000,
                  /*max_forward_jump*/ 1000000);

    auto seed = det.detect(0, 200);
    if (seed != WrapStatus::NoWrap) throw std::runtime_error("seed must be NoWrap");

    auto status = det.detect(200, 100);
    if (status != WrapStatus::BackwardJump) {
        throw std::runtime_error("expected BackwardJump for small backward delta");
    }

    std::printf("[B14] BackwardJump small delta PASS\n");
}

// Impossible: forward jump exceeding max_forward_jump.
static void test_b14_impossible_forward_jump() {
    FakePtsWrapDetector det;
    // Tight max forward jump so 100 -> 10000000 is flagged Impossible.
    det.configure(/*wrap_threshold*/ (1ll << 40),
                  /*backward_tolerance*/ 1000,
                  /*max_forward_jump*/ 1000);

    auto seed = det.detect(0, 100);
    if (seed != WrapStatus::NoWrap) throw std::runtime_error("seed must be NoWrap");

    auto status = det.detect(100, 10000000);
    if (status != WrapStatus::Impossible) {
        throw std::runtime_error("expected Impossible for huge forward jump");
    }

    std::printf("[B14] Impossible forward jump PASS\n");
}

// reset() clears prev; the next detect() seeds again and returns NoWrap.
static void test_b14_reset_clears_prev() {
    FakePtsWrapDetector det;
    det.configure(/*wrap_threshold*/ (1ll << 40),
                  /*backward_tolerance*/ 1000,
                  /*max_forward_jump*/ 1000000);

    auto seed = det.detect(0, 500);
    if (seed != WrapStatus::NoWrap) throw std::runtime_error("seed must be NoWrap");

    // Without reset, 500 -> 50 is a BackwardJump.
    auto before = det.detect(500, 50);
    if (before != WrapStatus::BackwardJump) {
        throw std::runtime_error("expected BackwardJump before reset");
    }

    // After reset, the next call seeds prev and returns NoWrap regardless of
    // the (now irrelevant) prev argument.
    det.reset();
    auto after = det.detect(99999, 50);
    if (after != WrapStatus::NoWrap) {
        throw std::runtime_error("first detect after reset must be NoWrap");
    }

    std::printf("[B14] reset clears prev PASS\n");
}

int main() {
    std::printf("=== P6B time_tests ===\n");

    P6B_RUN(test_b13_rescale_valid);
    P6B_RUN(test_b13_rescale_valid_simple_upscale);
    P6B_RUN(test_b13_rescale_overflow);
    P6B_RUN(test_b13_rescale_zero_denominator);
    P6B_RUN(test_b13_rescale_zero_src_num_yields_zero);
    P6B_RUN(test_b14_no_wrap_forward_progression);
    P6B_RUN(test_b14_forward_wrap);
    P6B_RUN(test_b14_backward_jump_small_delta);
    P6B_RUN(test_b14_impossible_forward_jump);
    P6B_RUN(test_b14_reset_clears_prev);

    if (g_failed == 0) {
        std::printf("=== P6B time_tests: ALL PASS ===\n");
        return 0;
    }
    std::printf("=== P6B time_tests: %d FAILED ===\n", g_failed);
    return 1;
}
