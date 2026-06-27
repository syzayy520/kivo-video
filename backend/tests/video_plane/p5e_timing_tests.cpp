// p5e_timing_tests.cpp -- P5E Software Decode / Timing Tests (E1-E10)
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <kivo/video_plane/decoder/fake_decoder.hpp>
#include <kivo/video_plane/timing/checked_rescale_runtime.hpp>
#include <kivo/video_plane/timing/timestamp_wrap.hpp>
#include <kivo/video_plane/timing/vfr_timing.hpp>
#include <kivo/video_plane/timing/bframe_reorder.hpp>
#include <kivo/video_plane/timing/seek_warmup.hpp>
#include <kivo/video_plane/timing/keyframe_rewind.hpp>
#include <kivo/video_plane/timing/recovery_bookmark.hpp>
#include <kivo/video_plane/timing/live_timestamp_runtime.hpp>
#include <kivo/video_plane/timing/low_latency_reorder.hpp>
#include <kivo/video_plane/recovery/recovery_seek_request.hpp>

static int g_failed = 0;

#define P5E_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// E1: Software decode path works on baseline corpus
static void test_e1_software_decode_path() {
    using namespace kivo::video_plane::decoder;
    DecoderContract c{};
    c.kind = DecoderKind::Fake;
    FakeDecoder dec(c);
    if (!dec.initialize()) throw 1;
    // Decode 10 frames as baseline corpus
    uint32_t keyframes = 0;
    uint32_t total = 0;
    for (int i = 0; i < 10; ++i) {
        auto f = dec.decode_next();
        if (!f.has_value()) throw 1;
        ++total;
        if (f->is_keyframe) ++keyframes;
    }
    if (total != 10) throw 1;
    if (keyframes == 0) throw 1; // at least one keyframe
    std::printf("[E1] SoftwareDecodePath PASS\n");
}

// E2: Checked rational rescale (int64 -> int128 -> TimestampOverflow)
static void test_e2_checked_rescale() {
    using namespace kivo::video_plane::timing;
    // Normal case: 1000/1 * 1/1000 = 1
    auto r1 = checked_rescale(1000, 1, 1, 1000);
    if (r1.overflow) throw 1;
    if (r1.value != 1) throw 1;
    // Larger but safe: (2^40) * 1 / 1 = 2^40
    auto r2 = checked_rescale(static_cast<int64_t>(1) << 40, 1, 1, 1);
    if (r2.overflow) throw 1;
    if (r2.value != (static_cast<int64_t>(1) << 40)) throw 1;
    // Overflow case: (2^62) * (2^62) / 1 -- exceeds int64 in multiply
    auto r3 = checked_rescale(static_cast<int64_t>(1) << 62, 1,
                               static_cast<int64_t>(1) << 62, 1);
    if (!r3.overflow) throw 1;
    if (r3.overflow_reason.find("TimestampOverflow") == std::string::npos) throw 1;
    // Division by zero
    auto r4 = checked_rescale(100, 0, 1, 1);
    if (!r4.overflow) throw 1;
    std::printf("[E2] CheckedRescale PASS\n");
}

// E3: Timestamp wrap tests
static void test_e3_timestamp_wrap() {
    using namespace kivo::video_plane::timing;
    // Normal sequence - no wrap
    TimestampWrapDetector det1;
    auto r1 = det1.process(1000);
    if (r1.wrapped) throw 1;
    auto r2 = det1.process(2000);
    if (r2.wrapped) throw 1;
    // Forward wrap: from near 32-bit max to near 0
    TimestampWrapDetector det2;
    auto r3 = det2.process(static_cast<int64_t>(0xFFFFFFF0)); // near 32-bit max
    if (r3.wrapped) throw 1;
    auto r4 = det2.process(static_cast<int64_t>(0x00000010)); // wrapped to near 0
    if (!r4.wrapped) throw 1;
    if (r4.wrap_direction != "forward") throw 1;
    if (det2.total_wraps() != 1) throw 1;
    // Unwrapped value should be > original (monotonic)
    if (r4.unwrapped_value <= r3.unwrapped_value) throw 1;
    std::printf("[E3] TimestampWrap PASS\n");
}

// E4: VFR timing tests
static void test_e4_vfr_timing() {
    using namespace kivo::video_plane::timing;
    FakeVfrTimingEngine engine;
    // CFR: all 33ms intervals
    engine.add_frame(0, 0);
    engine.add_frame(1, 33);
    engine.add_frame(2, 66);
    engine.compute_durations();
    if (engine.has_vfr()) throw 1;
    // VFR: mixed intervals
    FakeVfrTimingEngine engine2;
    engine2.add_frame(0, 0);
    engine2.add_frame(1, 33);
    engine2.add_frame(2, 100); // longer interval
    engine2.add_frame(3, 133);
    engine2.compute_durations();
    if (!engine2.has_vfr()) throw 1;
    auto f2 = engine2.get_frame(1);
    if (!f2.has_value()) throw 1;
    if (!f2->is_vfr) throw 1;
    std::printf("[E4] VfrTiming PASS\n");
}

// E5: B-frame reorder tests
static void test_e5_bframe_reorder() {
    using namespace kivo::video_plane::timing;
    FakeBFrameReorderer reorderer;
    reorderer.set_max_depth(4);
    // Decode order: I(0) P(3) B(1) B(2) -- presentation order: 0 1 2 3
    reorderer.push({0, 0, 0, true});
    reorderer.push({1, 3, 30, false});
    reorderer.push({2, 1, 10, false});
    reorderer.push({3, 2, 20, false});
    auto out = reorderer.flush_all();
    if (out.size() != 4) throw 1;
    // Should be in presentation order
    if (out[0].presentation_order != 0) throw 1;
    if (out[1].presentation_order != 1) throw 1;
    if (out[2].presentation_order != 2) throw 1;
    if (out[3].presentation_order != 3) throw 1;
    std::printf("[E5] BFrameReorder PASS\n");
}

// E6: Seek warmup discard tests
static void test_e6_seek_warmup() {
    using namespace kivo::video_plane::timing;
    SeekWarmupConfig cfg;
    cfg.max_discard_frames = 3;
    cfg.discard_until_keyframe = true;
    FakeSeekWarmup warmup(cfg);
    warmup.start_seek();
    if (warmup.state() != WarmupState::WarmupActive) throw 1;
    // Discard non-keyframes before keyframe
    if (!warmup.should_discard(0, false)) throw 1;
    if (!warmup.should_discard(1, false)) throw 1;
    // Keyframe -- should still be discarded (counts toward max) but keyframe_seen = true
    if (!warmup.should_discard(2, true)) throw 1;
    if (!warmup.keyframe_seen()) throw 1;
    // max_discard reached -> complete
    if (warmup.state() != WarmupState::WarmupComplete) throw 1;
    // After complete, no more discards
    if (warmup.should_discard(3, false)) throw 1;
    std::printf("[E6] SeekWarmup PASS\n");
}

// E7: Fallback rewind to keyframe tests
static void test_e7_keyframe_rewind() {
    using namespace kivo::video_plane::timing;
    FakeKeyframeIndex index;
    index.add_keyframe(0, 0);
    index.add_keyframe(30, 1000);
    index.add_keyframe(60, 2000);
    index.add_keyframe(90, 3000);
    // Rewind from pts 2500 -> should find keyframe at 2000
    auto r1 = index.rewind_to_keyframe(2500);
    if (!r1.has_value()) throw 1;
    if (r1->pts_ms != 2000) throw 1;
    // Rewind from pts 0 -> should find keyframe at 0
    auto r2 = index.rewind_to_keyframe(0);
    if (!r2.has_value()) throw 1;
    if (r2->pts_ms != 0) throw 1;
    // Rewind before first keyframe -> rewind to first
    auto r3 = index.rewind_to_keyframe(-100);
    if (!r3.has_value()) throw 1;
    if (r3->pts_ms != 0) throw 1;
    // Forward to keyframe from 2500 -> 3000
    auto r4 = index.forward_to_keyframe(2500);
    if (!r4.has_value()) throw 1;
    if (r4->pts_ms != 3000) throw 1;
    std::printf("[E7] KeyframeRewind PASS\n");
}

// E8: RecoveryBookmark and RecoverySeekRequest tests
static void test_e8_recovery_bookmark() {
    using namespace kivo::video_plane::timing;
    using namespace kivo::video_plane::recovery;
    FakeRecoverySeekEngine engine;
    // Create bookmark
    auto bm = engine.create_bookmark(50000, 42);
    if (!bm.is_valid) throw 1;
    if (bm.media_time_ms != 50000) throw 1;
    if (bm.pipeline_revision != 42) throw 1;
    // Submit seek with matching revision -> accepted
    RecoverySeekRequest req;
    req.timeline_revision = 42;
    req.target_media_time = 50000;
    req.user_visible = false;
    auto outcome = engine.submit_seek(req);
    if (outcome != RecoverySeekOutcome::Accepted) throw 1;
    // Submit seek with mismatched revision -> rejected
    RecoverySeekRequest req2;
    req2.timeline_revision = 99;
    auto outcome2 = engine.submit_seek(req2);
    if (outcome2 != RecoverySeekOutcome::RejectedByP4) throw 1;
    // Handle rejection -> standby
    auto outcome3 = engine.handle_rejection();
    if (outcome3 != RecoverySeekOutcome::StandbyUntilNewInput) throw 1;
    // RecoverySeek must be invisible
    if (engine.last_request().user_visible) throw 1;
    std::printf("[E8] RecoveryBookmark PASS\n");
}

// E9: Live timestamp origin resolver tests (7-level priority)
static void test_e9_live_timestamp_resolver() {
    using namespace kivo::video_plane::timing;
    // All sources available -> P4Approved wins
    FakeLiveTimestampResolver r1;
    r1.set_p4_approved(1000);
    r1.set_p4_timeline_hint(2000);
    r1.set_recovery_bookmark(3000);
    r1.set_segment_absolute(4000);
    r1.set_pcr_pts(5000);
    r1.set_conservative_rebase(6000);
    auto res1 = r1.resolve();
    if (res1.source != LiveTimestampSource::P4Approved) throw 1;
    if (res1.resolved_origin_ms != 1000) throw 1;
    // No P4Approved -> P4TimelineOriginHint wins
    FakeLiveTimestampResolver r2;
    r2.set_p4_timeline_hint(2000);
    r2.set_recovery_bookmark(3000);
    r2.set_segment_absolute(4000);
    auto res2 = r2.resolve();
    if (res2.source != LiveTimestampSource::P4TimelineOriginHint) throw 1;
    if (res2.resolved_origin_ms != 2000) throw 1;
    // No P4 sources -> RecoveryBookmark wins
    FakeLiveTimestampResolver r3;
    r3.set_recovery_bookmark(3000);
    r3.set_segment_absolute(4000);
    r3.set_pcr_pts(5000);
    auto res3 = r3.resolve();
    if (res3.source != LiveTimestampSource::RecoveryBookmark) throw 1;
    // No bookmark -> SegmentAbsoluteTime
    FakeLiveTimestampResolver r4;
    r4.set_segment_absolute(4000);
    r4.set_pcr_pts(5000);
    auto res4 = r4.resolve();
    if (res4.source != LiveTimestampSource::SegmentAbsoluteTime) throw 1;
    // No segment -> PcrPts
    FakeLiveTimestampResolver r5;
    r5.set_pcr_pts(5000);
    r5.set_conservative_rebase(6000);
    auto res5 = r5.resolve();
    if (res5.source != LiveTimestampSource::PcrPts) throw 1;
    // No PCR -> ConservativeLiveRebase
    FakeLiveTimestampResolver r6;
    r6.set_conservative_rebase(6000);
    auto res6 = r6.resolve();
    if (res6.source != LiveTimestampSource::ConservativeLiveRebase) throw 1;
    // Nothing -> TimestampInvalid
    FakeLiveTimestampResolver r7;
    auto res7 = r7.resolve();
    if (res7.source != LiveTimestampSource::TimestampInvalid) throw 1;
    if (res7.is_valid) throw 1;
    std::printf("[E9] LiveTimestampResolver PASS\n");
}

// E10: Low-latency reorder budget tests
static void test_e10_low_latency_reorder() {
    using namespace kivo::video_plane::timing;
    LowLatencyReorderConfig cfg;
    cfg.target_latency_budget_ms = 500;
    cfg.max_latency_ms = 2000;
    cfg.skip_to_live_edge_threshold_ms = 3000;
    // Within budget -> normal reorder
    auto r1 = evaluate_low_latency_reorder(300, cfg);
    if (r1.action != LowLatencyReorderAction::NormalReorder) throw 1;
    // Exceeds target but within max -> catch-up
    auto r2 = evaluate_low_latency_reorder(1000, cfg);
    if (r2.action != LowLatencyReorderAction::CatchUp) throw 1;
    // Exceeds max but within skip threshold -> skip to live edge
    auto r3 = evaluate_low_latency_reorder(2500, cfg);
    if (r3.action != LowLatencyReorderAction::SkipToLiveEdge) throw 1;
    // Exceeds skip threshold -> fail
    auto r4 = evaluate_low_latency_reorder(5000, cfg);
    if (r4.action != LowLatencyReorderAction::Fail) throw 1;
    std::printf("[E10] LowLatencyReorder PASS\n");
}

int main() {
    std::printf("=== P5E Software Decode / Timing Tests ===\n\n");
    P5E_RUN(test_e1_software_decode_path);
    P5E_RUN(test_e2_checked_rescale);
    P5E_RUN(test_e3_timestamp_wrap);
    P5E_RUN(test_e4_vfr_timing);
    P5E_RUN(test_e5_bframe_reorder);
    P5E_RUN(test_e6_seek_warmup);
    P5E_RUN(test_e7_keyframe_rewind);
    P5E_RUN(test_e8_recovery_bookmark);
    P5E_RUN(test_e9_live_timestamp_resolver);
    P5E_RUN(test_e10_low_latency_reorder);
    if (g_failed == 0) {
        std::printf("\n=== P5E Summary: all 10 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P5E Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
