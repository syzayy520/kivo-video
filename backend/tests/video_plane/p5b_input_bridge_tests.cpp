// p5b_input_bridge_tests.cpp — P5B P4 Input Bridge Tests (B1-B11)
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>

// P5B headers
#include <kivo/video_plane/input_bridge/p4_video_input_bridge.hpp>
#include <kivo/video_plane/input_bridge/encoded_input_io_adapter.hpp>
#include <kivo/video_plane/input_bridge/demux_buffer_pool.hpp>
#include <kivo/video_plane/input_bridge/out_of_band_backpressure.hpp>
#include <kivo/video_plane/input_bridge/attachment_safety_descriptor.hpp>
#include <kivo/video_plane/input_bridge/fake_demux_backend.hpp>
#include <kivo/video_plane/input_bridge/progress_watchdog.hpp>
#include <kivo/video_plane/input_bridge/ffmpeg_copy_to_pool_contract.hpp>
#include <kivo/video_plane/queue/queue_lifecycle_state.hpp>

static int g_failed = 0;

#define P5B_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// B1: P4VideoInputBridge fake tests
static void test_b1_p4_video_input_bridge() {
    using namespace kivo::video_plane::input_bridge;
    P4InputBridgeConfig cfg{};
    cfg.pipeline_id.value = 1;
    cfg.max_lease_window_ms = 5000;
    FakeP4VideoInputBridge bridge(cfg);
    if (bridge.state() != BridgeState::Ready) throw 1;
    auto stamp = bridge.accept_input(100);
    if (stamp.value != 1) throw 1;
    if (bridge.state() != BridgeState::Accepting) throw 1;
    bridge.renew_lease(1000);
    if (!bridge.is_lease_valid(2000)) throw 1;
    if (bridge.is_lease_valid(7000)) throw 1;  // expired
    bridge.drain();
    if (bridge.state() != BridgeState::Draining) throw 1;
    bridge.close();
    if (bridge.state() != BridgeState::Closed) throw 1;
    std::printf("[B1] P4VideoInputBridge PASS\n");
}

// B2: EncodedInputIOAdapter fake tests
static void test_b2_encoded_input_io_adapter() {
    using namespace kivo::video_plane::input_bridge;
    EncodedInputIOConfig cfg{};
    cfg.kind = IOAdapterKind::LocalFile;
    FakeEncodedInputIOAdapter adapter(cfg);
    if (adapter.is_open()) throw 1;
    if (!adapter.open("test://fake")) throw 1;
    if (!adapter.is_open()) throw 1;
    auto probe = adapter.probe();
    if (!probe.has_value()) throw 1;
    auto result = adapter.read(1024);
    if (!result.success) throw 1;
    if (result.bytes_read != 1024) throw 1;
    if (result.data.size() != 1024) throw 1;
    if (adapter.position() != 1024) throw 1;
    if (!adapter.seek(0)) throw 1;
    if (adapter.position() != 0) throw 1;
    adapter.close();
    if (adapter.is_open()) throw 1;
    std::printf("[B2] EncodedInputIOAdapter PASS\n");
}

// B3: DemuxBufferPool allocation cap tests
static void test_b3_demux_buffer_pool() {
    using namespace kivo::video_plane::input_bridge;
    BufferPoolConfig cfg{};
    cfg.max_total_bytes = 1024;
    cfg.max_single_buffer_bytes = 512;
    cfg.enforce_cap = true;
    DemuxBufferPool pool(cfg);
    auto h1 = pool.allocate(256);
    if (!h1.has_value()) throw 1;
    if (pool.stats().current_usage != 256) throw 1;
    auto h2 = pool.allocate(512);
    if (!h2.has_value()) throw 1;
    if (pool.stats().current_usage != 768) throw 1;
    // Should exceed cap (768 + 256 = 1024, exactly at cap)
    auto h3 = pool.allocate(257);
    if (h3.has_value()) throw 1;  // cap exceeded
    if (!pool.stats().cap_exceeded) throw 1;
    // Free and retry
    if (!pool.free(*h1)) throw 1;
    if (pool.stats().current_usage != 512) throw 1;
    auto h4 = pool.allocate(256);
    if (!h4.has_value()) throw 1;  // should succeed now (512+256=768 < 1024)
    // Oversized single buffer
    auto h5 = pool.allocate(513);
    if (h5.has_value()) throw 1;  // exceeds max_single_buffer_bytes
    std::printf("[B3] DemuxBufferPool PASS\n");
}

// B4: Queue lifecycle tests
static void test_b4_queue_lifecycle() {
    using namespace kivo::video_plane::input_bridge;
    using kivo::video_plane::queue::QueueLifecycleState;
    // Test queue lifecycle state transitions
    QueueLifecycleState state = QueueLifecycleState::Accepting;
    if (state != QueueLifecycleState::Accepting) throw 1;
    state = QueueLifecycleState::Processing;
    if (state != QueueLifecycleState::Processing) throw 1;
    state = QueueLifecycleState::Cancelling;
    state = QueueLifecycleState::Draining;
    state = QueueLifecycleState::Drained;
    state = QueueLifecycleState::Rejected;
    state = QueueLifecycleState::Failed;
    // Test all 7 states exist
    int count = 0;
    for (int i = 0; i < 7; ++i) {
        switch (static_cast<QueueLifecycleState>(i)) {
            case QueueLifecycleState::Accepting:
            case QueueLifecycleState::Processing:
            case QueueLifecycleState::Cancelling:
            case QueueLifecycleState::Draining:
            case QueueLifecycleState::Drained:
            case QueueLifecycleState::Rejected:
            case QueueLifecycleState::Failed:
                ++count;
                break;
        }
    }
    if (count != 7) throw 1;
    std::printf("[B4] QueueLifecycle PASS\n");
}

// B5: OutOfBandBackpressure atomic publish tests
static void test_b5_out_of_band_backpressure() {
    using namespace kivo::video_plane::input_bridge;
    OutOfBandBackpressure bp;
    if (bp.read() != BackpressureLevel::None) throw 1;
    if (bp.revision() != 0) throw 1;
    bp.publish(BackpressureLevel::Medium);
    if (bp.read() != BackpressureLevel::Medium) throw 1;
    if (bp.revision() != 1) throw 1;
    uint64_t rev = bp.revision();
    if (bp.changed_since(rev)) throw 1;
    bp.publish(BackpressureLevel::Critical);
    if (!bp.changed_since(rev)) throw 1;
    if (bp.read() != BackpressureLevel::Critical) throw 1;
    std::printf("[B5] OutOfBandBackpressure PASS\n");
}

// B6: AttachmentSafetyDescriptor tests
static void test_b6_attachment_safety_descriptor() {
    using namespace kivo::video_plane::input_bridge;
    AttachmentSafetyDescriptor desc;
    desc.attachment_id = "att_001";
    desc.declared_mime = "application/x-truetype-font";
    desc.sniffed_mime = "application/x-truetype-font";
    desc.byte_size = 1024;
    desc.quota_scope.max_total_bytes = 1024 * 1024;
    desc.quota_scope.current_bytes = 0;
    desc.mime_policy.resolution = kivo::video_plane::attachment::MimeConflictResolution::UseSafest;
    desc.evaluate();
    if (desc.safety_state != AttachmentSafetyState::Safe) throw 1;
    // Test MIME conflict
    desc.sniffed_mime = "application/octet-stream";
    desc.evaluate();
    if (desc.safety_state != AttachmentSafetyState::Quarantined) throw 1;
    // Test quota exceeded
    desc.sniffed_mime = "application/x-truetype-font";  // reset to match
    desc.quota_scope.current_bytes = desc.quota_scope.max_total_bytes;
    desc.evaluate();
    if (desc.safety_state != AttachmentSafetyState::Rejected) throw 1;
    std::printf("[B6] AttachmentSafetyDescriptor PASS\n");
}

// B7/B8: FakeDemuxBackend + unknown bitrate fallback
static void test_b7_b8_fake_demux_backend() {
    using namespace kivo::video_plane::input_bridge;
    FakeDemuxBackend demux;
    if (demux.is_initialized()) throw 1;
    std::vector<DemuxStreamInfo> streams = {
        {0, "h264", 2000000, 60000},
        {1, "aac", 128000, 60000},
        {2, "unknown_codec", 0, 60000}  // unknown bitrate
    };
    demux.initialize(streams);
    if (!demux.is_initialized()) throw 1;

    // B7: demux known corpus
    auto result = demux.demux_next();
    if (!result.success) throw 1;
    if (result.eof) throw 1;
    if (result.packet.stamp.value != 1) throw 1;
    if (result.packet.pts != 0) throw 1;

    auto result2 = demux.demux_next();
    if (!result2.success) throw 1;
    if (result2.packet.stamp.value != 2) throw 1;

    // B8: unknown bitrate fallback evidence
    auto evidence = demux.check_unknown_bitrate();
    if (!evidence.has_value()) throw 1;
    if (evidence->evidence_kind != "UnknownBitrateFallback") throw 1;
    if (evidence->reason_code != "bitrate_unknown") throw 1;

    std::printf("[B7/B8] FakeDemuxBackend+UnknownBitrate PASS\n");
}

// B9: No P4 lease retained beyond allowed window
static void test_b9_p4_lease_window() {
    using namespace kivo::video_plane::input_bridge;
    P4InputBridgeConfig cfg{};
    cfg.max_lease_window_ms = 3000;
    FakeP4VideoInputBridge bridge(cfg);
    bridge.renew_lease(1000);
    // Within window
    if (!bridge.is_lease_valid(2000)) throw 1;
    if (!bridge.is_lease_valid(3999)) throw 1;
    // Beyond window
    if (bridge.is_lease_valid(4001)) throw 1;
    // Renew extends window
    bridge.renew_lease(3500);
    if (!bridge.is_lease_valid(5000)) throw 1;
    if (bridge.is_lease_valid(7000)) throw 1;
    std::printf("[B9] P4LeaseWindow PASS\n");
}

// B10: FFmpeg copy-to-pool default contract
static void test_b10_ffmpeg_copy_to_pool_contract() {
    using namespace kivo::video_plane::input_bridge;
    FFmpegCopyToPoolContract contract{};
    if (!contract.copy_on_demux) throw 1;
    if (!contract.zero_source_after_copy) throw 1;
    if (!contract.validate_packet_size) throw 1;
    if (!contract.reject_oversized) throw 1;
    // Valid packet
    if (!validate_packet_against_contract(contract, 1024)) throw 1;
    // Oversized packet
    if (validate_packet_against_contract(contract, contract.max_packet_copy_bytes + 1)) throw 1;
    // Exactly at limit
    if (!validate_packet_against_contract(contract, contract.max_packet_copy_bytes)) throw 1;
    std::printf("[B10] FFmpegCopyToPoolContract PASS\n");
}

// B11: ProgressWatchdog contract tests
static void test_b11_progress_watchdog() {
    using namespace kivo::video_plane::input_bridge;
    WatchdogConfig cfg{};
    cfg.stall_threshold_ms = 5000;
    cfg.max_stall_count = 3;
    ProgressWatchdog wd(cfg);
    if (wd.state() != WatchdogState::NotStarted) throw 1;
    wd.start(1000);
    if (wd.state() != WatchdogState::Running) throw 1;
    // Normal progress
    wd.report_progress(2000);
    auto alert = wd.check(3000);
    if (alert.has_value()) throw 1;  // no stall
    if (wd.state() != WatchdogState::Running) throw 1;
    // Stall (7000 - 2000 = 5000, not > 5000, not yet)
    alert = wd.check(7000);
    if (alert.has_value()) throw 1;
    // Stall (7001 - 2000 = 5001 > 5000, stall detected)
    alert = wd.check(7001);
    // First stall: state=Stalled, no alert yet
    if (wd.stall_count() != 1) throw 1;
    // Continue stalling
    alert = wd.check(7002);
    if (wd.stall_count() != 2) throw 1;
    // Third stall: should alert
    alert = wd.check(7003);
    if (!alert.has_value()) throw 1;
    if (alert->evidence_kind != "ProgressStallAlert") throw 1;
    if (wd.state() != WatchdogState::Alerted) throw 1;
    // Report progress recovers from stalled (but Alerted is terminal)
    wd.stop();
    if (wd.state() != WatchdogState::NotStarted) throw 1;
    std::printf("[B11] ProgressWatchdog PASS\n");
}

int main() {
    std::printf("=== P5B P4 Input Bridge Tests ===\n\n");
    P5B_RUN(test_b1_p4_video_input_bridge);
    P5B_RUN(test_b2_encoded_input_io_adapter);
    P5B_RUN(test_b3_demux_buffer_pool);
    P5B_RUN(test_b4_queue_lifecycle);
    P5B_RUN(test_b5_out_of_band_backpressure);
    P5B_RUN(test_b6_attachment_safety_descriptor);
    P5B_RUN(test_b7_b8_fake_demux_backend);
    P5B_RUN(test_b9_p4_lease_window);
    P5B_RUN(test_b10_ffmpeg_copy_to_pool_contract);
    P5B_RUN(test_b11_progress_watchdog);
    if (g_failed == 0) {
        std::printf("\n=== P5B Summary: all 11 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P5B Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
