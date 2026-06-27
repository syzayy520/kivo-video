// p5d_decoder_tests.cpp — P5D Decoder Contract Tests (D1-D9)
#include <cstdint>
#include <cstdio>
#include <string>

#include <kivo/video_plane/decoder/decoder_contract.hpp>
#include <kivo/video_plane/decoder/fake_decoder.hpp>
#include <kivo/video_plane/decoder/frame_lease.hpp>
#include <kivo/video_plane/decoder/frame_pool.hpp>
#include <kivo/video_plane/decoder/render_thread_destructor_policy.hpp>
#include <kivo/video_plane/decoder/queue_drain_escape.hpp>
#include <kivo/video_plane/decoder/secure_decode_lifecycle.hpp>
#include <kivo/video_plane/decoder/protected_export_denied.hpp>
#include <kivo/video_plane/thread/com_release_policy.hpp>
#include <kivo/video_plane/protected_content/protected_boundary_hint.hpp>
#include <kivo/video_plane/export/frame_export_security_policy.hpp>

static int g_failed = 0;

#define P5D_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// D1: Decoder contract complete
static void test_d1_decoder_contract() {
    using namespace kivo::video_plane::decoder;
    DecoderContract c{};
    if (c.kind != DecoderKind::Fake) throw 1;
    if (c.max_frame_queue_depth != 16) throw 1;
    if (!c.supports_drain) throw 1;
    if (!c.supports_flush) throw 1;
    std::printf("[D1] DecoderContract PASS\n");
}

// D2: Fake decoder produces frames
static void test_d2_fake_decoder() {
    using namespace kivo::video_plane::decoder;
    DecoderContract c{};
    FakeDecoder dec(c);
    if (!dec.initialize()) throw 1;
    auto f1 = dec.decode_next();
    if (!f1.has_value()) throw 1;
    if (f1->stamp.value != 1) throw 1;
    if (f1->width != 1920) throw 1;
    if (f1->height != 1080) throw 1;
    if (!f1->is_keyframe) throw 1;  // frame 0 is keyframe
    auto f2 = dec.decode_next();
    if (!f2.has_value()) throw 1;
    if (f2->is_keyframe) throw 1;  // frame 1 is not keyframe
    if (dec.frame_count() != 2) throw 1;
    dec.flush();
    if (dec.frame_count() != 0) throw 1;
    std::printf("[D2] FakeDecoder PASS\n");
}

// D3: FrameLease / RetireToken
static void test_d3_frame_lease() {
    using namespace kivo::video_plane::decoder;
    RetireToken t{};
    if (t.is_valid()) throw 1;
    t.value = 42;
    if (!t.is_valid()) throw 1;
    FrameLease lease{};
    lease.frame_handle = 1;
    lease.retire_token = t;
    lease.leased_at_ms = 1000;
    lease.max_lease_ms = 5000;
    if (lease.is_expired(3000)) throw 1;
    if (!lease.is_expired(7000)) throw 1;
    lease.add_ref();
    lease.add_ref();
    if (lease.refs() != 2) throw 1;
    lease.release();
    if (lease.refs() != 1) throw 1;
    std::printf("[D3] FrameLease/RetireToken PASS\n");
}

// D4: FramePool recycle
static void test_d4_frame_pool() {
    using namespace kivo::video_plane::decoder;
    FramePoolConfig cfg{};
    cfg.max_frames = 4;
    FakeFramePool pool(cfg);
    auto h1 = pool.acquire();
    auto h2 = pool.acquire();
    auto h3 = pool.acquire();
    auto h4 = pool.acquire();
    if (pool.in_use_count() != 4) throw 1;
    // Pool exhausted
    auto h5 = pool.acquire();
    if (h5 != 0) throw 1;
    if (!pool.is_exhausted()) throw 1;
    // Release and recycle
    pool.release(h2);
    if (pool.in_use_count() != 3) throw 1;
    if (pool.recycled_count() != 1) throw 1;
    // Acquire should get recycled handle
    auto h6 = pool.acquire();
    if (h6 != h2) throw 1;  // should be recycled
    if (pool.recycled_count() != 0) throw 1;
    std::printf("[D4] FramePool PASS\n");
}

// D5: No heavy destructor on RenderThread
static void test_d5_render_thread_destructor() {
    using namespace kivo::video_plane::decoder;
    DestructorPolicy pol{};
    if (!pol.defer_heavy_destructors) throw 1;
    if (pol.max_render_thread_destructor_ms != 1) throw 1;
    // 0ms destructor is fine
    if (is_destructor_too_heavy(pol, 0)) throw 1;
    // 1ms destructor is fine (at limit)
    if (is_destructor_too_heavy(pol, 1)) throw 1;
    // 2ms destructor is too heavy
    if (!is_destructor_too_heavy(pol, 2)) throw 1;
    std::printf("[D5] RenderThreadDestructor PASS\n");
}

// D6: Queue drain escape hatch
static void test_d6_queue_drain_escape() {
    using namespace kivo::video_plane::decoder;
    DrainEscapeConfig cfg{};
    cfg.max_drain_ms = 5000;
    FakeQueueDrainEscape escape(cfg);
    if (escape.state() != DrainState::NotStarted) throw 1;
    escape.start_drain();
    if (escape.state() != DrainState::Draining) throw 1;
    if (escape.check(3000) != DrainState::Draining) throw 1;
    if (escape.check(5001) != DrainState::ForceTerminated) throw 1;
    // Test complete path
    FakeQueueDrainEscape escape2(cfg);
    escape2.start_drain();
    escape2.complete();
    if (escape2.state() != DrainState::Completed) throw 1;
    std::printf("[D6] QueueDrainEscape PASS\n");
}

// D7: SecureDecodeSessionContext BoundaryOnly lifecycle
static void test_d7_secure_decode_lifecycle() {
    using namespace kivo::video_plane::decoder;
    FakeSecureDecodeLifecycle lc;
    if (lc.state() != SecureDecodeState::NotCreated) throw 1;
    auto h = lc.create();
    if (!h.is_valid()) throw 1;
    if (lc.state() != SecureDecodeState::Created) throw 1;
    if (!lc.is_boundary_safe()) throw 1;
    lc.activate();
    if (lc.state() != SecureDecodeState::Active) throw 1;
    lc.suspend();
    if (lc.state() != SecureDecodeState::Suspended) throw 1;
    lc.resume();
    if (lc.state() != SecureDecodeState::Active) throw 1;
    lc.destroy();
    if (lc.state() != SecureDecodeState::Destroyed) throw 1;
    if (lc.handle().is_valid()) throw 1;  // handle invalidated
    std::printf("[D7] SecureDecodeLifecycle PASS\n");
}

// D8: Protected export denied
static void test_d8_protected_export_denied() {
    using namespace kivo::video_plane::decoder;
    using namespace kivo::video_plane::protected_content;
    using namespace kivo::video_plane::export_policy;
    // Non-protected content - export allowed
    ProtectedBoundaryHint hint{};
    FrameExportSecurityPolicy pol{};
    auto r1 = check_protected_export(hint, pol);
    if (r1.denied) throw 1;
    // Protected content - export denied
    hint.kind = ProtectedBoundaryKind::HDCP;
    hint.requires_secure_decode = true;
    auto r2 = check_protected_export(hint, pol);
    if (!r2.denied) throw 1;
    if (r2.reason != "protected_content_export_denied") throw 1;
    std::printf("[D8] ProtectedExportDenied PASS\n");
}

// D9: COM release policy
static void test_d9_com_release_policy() {
    using namespace kivo::video_plane::thread;
    ComReleasePolicy pol{};
    if (!pol.defer_release) throw 1;
    if (!pol.release_on_thread_exit) throw 1;
    if (pol.max_deferred_count != 64) throw 1;
    if (pol.release_thread_name.empty()) throw 1;
    std::printf("[D9] ComReleasePolicy PASS\n");
}

int main() {
    std::printf("=== P5D Decoder Contract Tests ===\n\n");
    P5D_RUN(test_d1_decoder_contract);
    P5D_RUN(test_d2_fake_decoder);
    P5D_RUN(test_d3_frame_lease);
    P5D_RUN(test_d4_frame_pool);
    P5D_RUN(test_d5_render_thread_destructor);
    P5D_RUN(test_d6_queue_drain_escape);
    P5D_RUN(test_d7_secure_decode_lifecycle);
    P5D_RUN(test_d8_protected_export_denied);
    P5D_RUN(test_d9_com_release_policy);
    if (g_failed == 0) {
        std::printf("\n=== P5D Summary: all 9 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P5D Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
