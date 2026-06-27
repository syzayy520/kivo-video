// p5g_d3d11va_p010_tests.cpp -- P5G D3D11VA / P010 / GPU Tests (G1-G13)
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <kivo/video_plane/gpu_hw/d3d11va_hwcontext.hpp>
#include <kivo/video_plane/gpu_hw/array_texture_resolver.hpp>
#include <kivo/video_plane/gpu_hw/context_lock_bridge.hpp>
#include <kivo/video_plane/gpu_hw/nv12_srv.hpp>
#include <kivo/video_plane/gpu_hw/p010_integer_gate.hpp>
#include <kivo/video_plane/gpu_hw/p010_unorm_gate.hpp>
#include <kivo/video_plane/gpu_hw/p010_golden.hpp>
#include <kivo/video_plane/gpu_hw/srv_fallback.hpp>
#include <kivo/video_plane/gpu_hw/adapter_topology.hpp>
#include <kivo/video_plane/gpu_hw/cross_adapter_ban.hpp>
#include <kivo/video_plane/gpu_hw/shared_texture_gate.hpp>
#include <kivo/video_plane/gpu_hw/tier_a_evidence.hpp>
#include <kivo/video_plane/gpu_hw/gpu_budget_runtime.hpp>

static int g_failed = 0;

#define P5G_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// G1: D3D11VA hwcontext bridge
static void test_g1_d3d11va_hwcontext() {
    using namespace kivo::video_plane::gpu_hw;
    FakeD3D11vaHwcontext ctx;
    if (ctx.is_valid()) throw 1;
    if (!ctx.initialize(0x1234)) throw 1;
    if (!ctx.is_valid()) throw 1;
    if (ctx.device_epoch() == 0) throw 1;
    ctx.simulate_device_lost();
    if (ctx.state() != HwcontextState::DeviceLost) throw 1;
    ctx.release();
    if (!ctx.initialize(0x5678)) throw 1;
    std::printf("[G1] D3D11vaHwcontext PASS\n");
}

// G2: Array texture / slice resolver
static void test_g2_array_texture() {
    using namespace kivo::video_plane::gpu_hw;
    FakeArrayTextureResolver resolver;
    resolver.configure(4, 3); // 4 array slices, 3 mip levels
    auto s1 = resolver.resolve(0, 0);
    if (!s1.is_valid) throw 1;
    auto s2 = resolver.resolve(3, 2);
    if (!s2.is_valid) throw 1;
    auto s3 = resolver.resolve(4, 0); // out of bounds
    if (s3.is_valid) throw 1;
    auto s4 = resolver.resolve(0, 3); // out of bounds
    if (s4.is_valid) throw 1;
    std::printf("[G2] ArrayTexture PASS\n");
}

// G3: D3D11 context lock bridge
static void test_g3_context_lock() {
    using namespace kivo::video_plane::gpu_hw;
    FakeContextLockBridge lock;
    if (lock.is_locked()) throw 1;
    auto r1 = lock.try_lock(true, false); // GPU device thread
    if (r1 != ContextLockState::LockedByGpuDeviceThread) throw 1;
    if (!lock.is_locked()) throw 1;
    // Cannot re-lock
    auto r2 = lock.try_lock(true, false);
    if (r2 != ContextLockState::LockFailed) throw 1;
    lock.unlock();
    // Maintenance context can lock
    auto r3 = lock.try_lock(false, true);
    if (r3 != ContextLockState::LockedByMaintenanceThread) throw 1;
    lock.unlock();
    // Non-GPU non-maintenance cannot lock
    auto r4 = lock.try_lock(false, false);
    if (r4 != ContextLockState::LockFailed) throw 1;
    std::printf("[G3] ContextLock PASS\n");
}

// G4: NV12 plane SRV
static void test_g4_nv12_srv() {
    using namespace kivo::video_plane::gpu_hw;
    FakeNv12SrvCreator creator;
    auto r = creator.create_srvs(1920, 1080);
    if (!r.luma_srv_created) throw 1;
    if (!r.chroma_srv_created) throw 1;
    if (r.luma_plane_width != 1920) throw 1;
    if (r.luma_plane_height != 1080) throw 1;
    if (r.chroma_plane_width != 960) throw 1;
    if (r.chroma_plane_height != 540) throw 1;
    // Failure case
    auto r2 = creator.simulate_failure();
    if (r2.luma_srv_created) throw 1;
    if (r2.failure_reason.empty()) throw 1;
    std::printf("[G4] Nv12Srv PASS\n");
}

// G5: P010 IntegerPlaneViewVerified gate
static void test_g5_p010_integer_gate() {
    using namespace kivo::video_plane::gpu_hw;
    // stored_word = code10 << 6, e.g. code10=512 -> stored_word=0x8000
    uint16_t stored = 512 << 6; // 0x8000 = 32768
    auto r = verify_p010_integer_plane(stored);
    if (!r.verified) throw 1;
    if (r.code10 != 512) throw 1;
    if (r.evidence_id.empty()) throw 1;
    // Batch verify
    uint16_t samples[] = {64 << 6, 940 << 6, 0 << 6, 1023 << 6};
    if (!verify_p010_integer_batch(samples, 4)) throw 1;
    std::printf("[G5] P010IntegerGate PASS\n");
}

// G6: P010 UnormPlaneViewVerified gate
static void test_g6_p010_unorm_gate() {
    using namespace kivo::video_plane::gpu_hw;
    // reference: stored_word = 512 << 6 = 32768, code10 = 512
    uint16_t ref_stored = 512 << 6;
    // UNORM sampled value should be close to 32768
    uint16_t sampled = 32768;
    auto r = verify_p010_unorm_plane(sampled, ref_stored);
    if (!r.verified) throw 1;
    if (!r.within_tolerance) throw 1;
    if (r.code10_diff > 1 || r.code10_diff < -1) throw 1;
    // Out of tolerance
    uint16_t sampled_bad = 40000;
    auto r2 = verify_p010_unorm_plane(sampled_bad, ref_stored);
    if (r2.verified) throw 1;
    if (r2.within_tolerance) throw 1;
    std::printf("[G6] P010UnormGate PASS\n");
}

// G7: P010 golden numeric tests
static void test_g7_p010_golden() {
    using namespace kivo::video_plane::gpu_hw;
    // Y=512 (mid gray), Cb=512 (neutral), Cr=512 (neutral)
    auto r = p010_golden_test(512, 512, 512);
    if (!r.pass) throw 1;
    if (r.max_diff > 1e-9) throw 1;
    // Clamping test: Y=0 -> clamped to 64
    auto r2 = p010_golden_test(0, 0, 0);
    if (!r2.pass) throw 1;
    if (r2.y_limited < 64.0/1023.0 - 1e-9) throw 1;
    // Clamping test: Y=1023 -> clamped to 940
    auto r3 = p010_golden_test(1023, 1023, 1023);
    if (!r3.pass) throw 1;
    if (r3.y_limited > 940.0/1023.0 + 1e-9) throw 1;
    std::printf("[G7] P010Golden PASS\n");
}

// G8: SRV creation failure fallback
static void test_g8_srv_fallback() {
    using namespace kivo::video_plane::gpu_hw;
    FakeSrvFallbackHandler handler;
    auto r1 = handler.handle_failure("format_unsupported");
    if (r1.action != SrvFallbackAction::RetryWithDifferentFormat) throw 1;
    if (!r1.fallback_succeeded) throw 1;
    auto r2 = handler.handle_failure("device_removed");
    if (r2.action != SrvFallbackAction::FailToUpperLayer) throw 1;
    if (r2.fallback_succeeded) throw 1;
    auto r3 = handler.handle_failure("unknown_error");
    if (r3.action != SrvFallbackAction::FallbackToCpuCopyback) throw 1;
    if (!r3.fallback_succeeded) throw 1;
    std::printf("[G8] SrvFallback PASS\n");
}

// G9: Adapter topology
static void test_g9_adapter_topology() {
    using namespace kivo::video_plane::gpu_hw;
    FakeAdapterTopology topo;
    topo.add_adapter(0x1234, "NVIDIA", 0x2484, true, true);
    topo.add_adapter(0x5678, "Intel", 0x9A49, false, true);
    if (topo.adapter_count() != 2) throw 1;
    if (!topo.is_multi_gpu()) throw 1;
    auto* primary = topo.primary_adapter();
    if (!primary) throw 1;
    if (primary->luid != 0x1234) throw 1;
    auto* found = topo.find_adapter(0x5678);
    if (!found) throw 1;
    if (found->vendor_name != "Intel") throw 1;
    std::printf("[G9] AdapterTopology PASS\n");
}

// G10: Cross-adapter production ban
static void test_g10_cross_adapter_ban() {
    using namespace kivo::video_plane::gpu_hw;
    FakeCrossAdapterBan ban;
    // Same adapter -> OK
    auto r1 = ban.check(0x1234, 0x1234);
    if (r1.is_banned) throw 1;
    // Different adapter -> BANNED
    auto r2 = ban.check(0x1234, 0x5678);
    if (!r2.is_banned) throw 1;
    if (r2.evidence_id.empty()) throw 1;
    // Experimental in production -> BANNED
    auto r3 = ban.check_experimental(true);
    if (!r3.is_banned) throw 1;
    // Experimental in lab -> OK
    auto r4 = ban.check_experimental(false);
    if (r4.is_banned) throw 1;
    std::printf("[G10] CrossAdapterBan PASS\n");
}

// G11: Shared texture experimental gate remains off
static void test_g11_shared_texture_gate() {
    using namespace kivo::video_plane::gpu_hw;
    FakeSharedTextureGate gate;
    // Production -> gate OFF
    auto r1 = gate.check(false, true);
    if (r1.state != SharedTextureGateState::Off) throw 1;
    if (r1.is_allowed) throw 1;
    // Lab -> gate LabOnly
    auto r2 = gate.check(true, false);
    if (r2.state != SharedTextureGateState::LabOnly) throw 1;
    if (!r2.is_allowed) throw 1;
    // Verify production gate off
    if (!gate.verify_production_off()) throw 1;
    std::printf("[G11] SharedTextureGate PASS\n");
}

// G12: Tier A real hardware evidence or safe-fail
static void test_g12_tier_a_evidence() {
    using namespace kivo::video_plane::gpu_hw;
    FakeTierAValidator validator;
    // Real GPU -> hardware verified
    auto r1 = validator.validate(true, "NVIDIA RTX 3080");
    if (r1.status != TierAStatus::HardwareVerified) throw 1;
    if (!validator.is_acceptable(r1)) throw 1;
    // No GPU -> safe fail
    auto r2 = validator.validate(false, "");
    if (r2.status != TierAStatus::SafeFail) throw 1;
    if (!validator.is_acceptable(r2)) throw 1;
    if (r2.safe_fail_reason.empty()) throw 1;
    // Missing evidence -> failure
    TierAEvidence missing;
    missing.status = TierAStatus::MissingEvidence;
    if (!validator.is_failure(missing)) throw 1;
    std::printf("[G12] TierAEvidence PASS\n");
}

// G13: GPU budget provider
static void test_g13_gpu_budget() {
    using namespace kivo::video_plane::gpu_hw;
    using namespace kivo::video_plane::gpu;
    FakeGpuBudgetProvider provider;
    provider.set_budget(8ULL * 1024 * 1024 * 1024, 16, 8, 4); // 8GB, 16 surfaces, 8 RTs, 4 BBs
    // Relaxed: 2GB used
    auto r1 = provider.check(2ULL * 1024 * 1024 * 1024, 4, 2, 1);
    if (r1.pressure != BudgetPressureLevel::Relaxed) throw 1;
    if (!r1.is_within_budget) throw 1;
    // Critical: exceeds decode surfaces
    auto r2 = provider.check(2ULL * 1024 * 1024 * 1024, 20, 2, 1);
    if (r2.pressure != BudgetPressureLevel::Critical) throw 1;
    if (r2.is_within_budget) throw 1;
    // High: ~87% memory (7GB of 8GB)
    auto r3 = provider.check(7ULL * 1024 * 1024 * 1024, 4, 2, 1);
    if (r3.pressure != BudgetPressureLevel::High) throw 1;
    std::printf("[G13] GpuBudget PASS\n");
}

int main() {
    std::printf("=== P5G D3D11VA / P010 / GPU Tests ===\n\n");
    P5G_RUN(test_g1_d3d11va_hwcontext);
    P5G_RUN(test_g2_array_texture);
    P5G_RUN(test_g3_context_lock);
    P5G_RUN(test_g4_nv12_srv);
    P5G_RUN(test_g5_p010_integer_gate);
    P5G_RUN(test_g6_p010_unorm_gate);
    P5G_RUN(test_g7_p010_golden);
    P5G_RUN(test_g8_srv_fallback);
    P5G_RUN(test_g9_adapter_topology);
    P5G_RUN(test_g10_cross_adapter_ban);
    P5G_RUN(test_g11_shared_texture_gate);
    P5G_RUN(test_g12_tier_a_evidence);
    P5G_RUN(test_g13_gpu_budget);
    if (g_failed == 0) {
        std::printf("\n=== P5G Summary: all 13 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P5G Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
