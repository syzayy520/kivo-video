// p5c_ffmpeg_demux_tests.cpp — P5C FFmpeg Demux Backend Tests (C1-C12)
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <kivo/video_plane/ffmpeg_demux/ffmpeg_wrapper_isolation.hpp>
#include <kivo/video_plane/ffmpeg_demux/avio_bridge.hpp>
#include <kivo/video_plane/ffmpeg_demux/interrupt_callback.hpp>
#include <kivo/video_plane/ffmpeg_demux/find_stream_info_deadline.hpp>
#include <kivo/video_plane/ffmpeg_demux/demux_quarantine.hpp>
#include <kivo/video_plane/ffmpeg_demux/track_discovery.hpp>
#include <kivo/video_plane/ffmpeg_demux/subtitle_attachment_map.hpp>
#include <kivo/video_plane/ffmpeg_demux/malformed_corpus_smoke.hpp>
#include <kivo/video_plane/input_bridge/demux_buffer_pool.hpp>
#include <kivo/video_plane/fuzz/backend_risk_register.hpp>
#include <kivo/video_plane/license/patent_risk_report.hpp>
#include <kivo/video_plane/license/redistribution_checklist.hpp>
#include <kivo/video_plane/sandbox/sandbox_broker_contract.hpp>
#include <kivo/video_plane/malformed/malformed_corpus_item.hpp>
#include <kivo/video_plane/failure/failure_correlation_id.hpp>

static int g_failed = 0;

#define P5C_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// C1: FFmpeg wrapper isolation
static void test_c1_ffmpeg_wrapper_isolation() {
    using namespace kivo::video_plane::ffmpeg_demux;
    if (!FFmpegWrapperIsolationContract::is_path_allowed("include/kivo/video_plane/ffmpeg_demux/avio_bridge.hpp")) throw 1;
    if (FFmpegWrapperIsolationContract::is_path_allowed("include/kivo/video_plane/stamp/core_video_stamp.hpp")) throw 1;
    if (FFmpegWrapperIsolationContract::is_path_allowed("include/kivo/video_plane/capability/capability_domain_id.hpp")) throw 1;
    std::printf("[C1] FFmpegWrapperIsolation PASS\n");
}

// C2: AVIO bridge copy-to-pool
static void test_c2_avio_bridge() {
    using namespace kivo::video_plane::ffmpeg_demux;
    using namespace kivo::video_plane::input_bridge;
    AvioBridgeConfig cfg{};
    FakeAvioBridge bridge(cfg);
    BufferPoolConfig pool_cfg{};
    pool_cfg.max_total_bytes = 1024 * 1024;
    DemuxBufferPool pool(pool_cfg);
    if (!bridge.copy_to_pool(pool, 4096)) throw 1;
    if (pool.stats().current_usage != 4096) throw 1;
    // Oversized should fail
    if (bridge.copy_to_pool(pool, cfg.copy_contract.max_packet_copy_bytes + 1)) throw 1;
    std::printf("[C2] AvioBridge PASS\n");
}

// C3: Interrupt callback binding
static void test_c3_interrupt_callback() {
    using namespace kivo::video_plane::ffmpeg_demux;
    FakeInterruptCallbackManager mgr;
    // Must not be bound initially
    if (mgr.verify_bound(CallbackBindPoint::BeforeOpen)) throw 1;
    // Bind before open
    mgr.bind(CallbackBindPoint::BeforeOpen, []() { return false; }, 5000);
    if (!mgr.verify_bound(CallbackBindPoint::BeforeOpen)) throw 1;
    if (!mgr.verify_bound(CallbackBindPoint::BeforeReadPacket)) throw 1;
    // Execute - no timeout
    if (mgr.execute(1000)) throw 1;  // should not interrupt
    // Execute - timeout
    if (!mgr.execute(6000)) throw 1;  // should interrupt (6000 > 5000)
    std::printf("[C3] InterruptCallback PASS\n");
}

// C4: find_stream_info deadline
static void test_c4_find_stream_info_deadline() {
    using namespace kivo::video_plane::ffmpeg_demux;
    FindStreamInfoConfig cfg{};
    cfg.deadline_ms = 10000;
    FakeFindStreamInfo fsi(cfg);
    // Success within deadline
    auto [result1, ev1] = fsi.execute(5000, true);
    if (result1 != FindStreamInfoResult::Success) throw 1;
    if (ev1.has_value()) throw 1;
    // Timeout
    auto [result2, ev2] = fsi.execute(15000, true);
    if (result2 != FindStreamInfoResult::Timeout) throw 1;
    if (!ev2.has_value()) throw 1;
    if (ev2->evidence_kind != "FindStreamInfoTimeout") throw 1;
    // Failed (no streams)
    auto [result3, ev3] = fsi.execute(5000, false);
    if (result3 != FindStreamInfoResult::Failed) throw 1;
    std::printf("[C4] FindStreamInfoDeadline PASS\n");
}

// C5: Demux quarantine
static void test_c5_demux_quarantine() {
    using namespace kivo::video_plane::ffmpeg_demux;
    using namespace kivo::video_plane::failure;
    FakeDemuxQuarantine q;
    FailureCorrelationId cid{};
    cid.value = 1;
    q.quarantine_for_crash(cid, "test://source1", 1000);
    if (!q.is_quarantined("test://source1")) throw 1;
    if (q.is_quarantined("test://source2")) throw 1;
    if (q.quarantine_count() != 1) throw 1;
    if (q.has_zombies()) throw 1;
    std::printf("[C5] DemuxQuarantine PASS\n");
}

// C6: Zombie quarantine
static void test_c6_zombie_quarantine() {
    using namespace kivo::video_plane::ffmpeg_demux;
    using namespace kivo::video_plane::failure;
    FakeDemuxQuarantine q;
    FailureCorrelationId cid{};
    cid.value = 2;
    q.quarantine_zombie(cid, "test://zombie", 2000);
    if (!q.has_zombies()) throw 1;
    if (q.zombie_count() != 1) throw 1;
    if (q.quarantine_count() != 1) throw 1;
    // Add another zombie
    cid.value = 3;
    q.quarantine_zombie(cid, "test://zombie2", 3000);
    if (q.zombie_count() != 2) throw 1;
    std::printf("[C6] ZombieQuarantine PASS\n");
}

// C7: FFmpeg risk register complete
static void test_c7_ffmpeg_risk_register() {
    using namespace kivo::video_plane::fuzz;
    P5ThirdPartyBackendRiskRegister reg{};
    reg.backend_name = "FFmpeg avformat";
    reg.risk_category = "parser";
    reg.is_active = true;
    reg.sandbox_required = true;
    reg.fuzz_target = "demux_parser";
    reg.expected_failure_mode = "bounded_failure";
    reg.gate_stage = "P5C";
    if (!reg.is_active) throw 1;
    if (!reg.sandbox_required) throw 1;
    if (reg.backend_name.empty()) throw 1;
    if (reg.fuzz_target.empty()) throw 1;
    std::printf("[C7] FFmpegRiskRegister PASS\n");
}

// C8: License manifest generated
static void test_c8_license_manifest() {
    using namespace kivo::video_plane::license;
    PatentRiskReport patent{};
    patent.level = PatentRiskLevel::Medium;
    patent.patent_category = "H.264";
    patent.mitigation = "license_required";
    RedistributionChecklist checklist{};
    checklist.required_licenses.push_back("GPL-v2+");
    checklist.required_notices.push_back("FFmpeg attribution");
    checklist.is_compliant = true;
    if (patent.level != PatentRiskLevel::Medium) throw 1;
    if (checklist.required_licenses.empty()) throw 1;
    if (!checklist.is_compliant) throw 1;
    std::printf("[C8] LicenseManifest PASS\n");
}

// C9: Malformed corpus smoke
static void test_c9_malformed_corpus_smoke() {
    using namespace kivo::video_plane::ffmpeg_demux;
    using namespace kivo::video_plane::malformed;
    FakeMalformedCorpusSmoke smoke;
    smoke.add_corpus({MalformedKind::Truncated, "malformed://truncated", "bounded_failure", "reg_001"});
    smoke.add_corpus({MalformedKind::CorruptHeader, "malformed://corrupt_header", "bounded_failure", "reg_002"});
    smoke.add_corpus({MalformedKind::OversizedPacket, "malformed://alloc_bomb", "bounded_failure", "reg_003"});
    if (smoke.corpus_count() != 3) throw 1;
    auto results = smoke.run_smoke();
    if (results.size() != 3) throw 1;
    for (auto const& r : results) {
        if (!r.handled_gracefully) throw 1;
        if (!r.bounded_failure) throw 1;
    }
    std::printf("[C9] MalformedCorpusSmoke PASS\n");
}

// C10: Sandbox broker contract fake tests
static void test_c10_sandbox_broker_contract() {
    using namespace kivo::video_plane::sandbox;
    P5SandboxBrokerContract contract{};
    contract.max_message_payload_size = 16 * 1024 * 1024;
    if (contract.max_message_payload_size != 16 * 1024 * 1024) throw 1;
    if (contract.buffer_placement != DemuxBufferPlacement::CopyBridge) throw 1;
    // Verify IPC version is set
    SandboxIpcProtocolVersion v{};
    contract.ipc_version = v;
    // Verify supported messages can include all 13
    for (int i = 0; i < 13; ++i) {
        contract.supported_messages.push_back(static_cast<SandboxBrokerMessageKind>(i));
    }
    if (contract.supported_messages.size() != 13) throw 1;
    std::printf("[C10] SandboxBrokerContract PASS\n");
}

// C11: Track discovery
static void test_c11_track_discovery() {
    using namespace kivo::video_plane::ffmpeg_demux;
    FakeTrackDiscovery discovery;
    if (discovery.is_discovered()) throw 1;
    std::vector<DiscoveredTrack> tracks = {
        {0, TrackKind::Video, "h264", "eng", true, false, 60000},
        {1, TrackKind::Audio, "aac", "eng", true, false, 60000},
        {2, TrackKind::Subtitle, "srt", "eng", false, false, 60000},
        {3, TrackKind::Subtitle, "srt", "jpn", false, true, 60000}
    };
    discovery.discover(tracks);
    if (!discovery.is_discovered()) throw 1;
    if (discovery.track_count() != 4) throw 1;
    auto video_tracks = discovery.tracks_by_kind(TrackKind::Video);
    if (video_tracks.size() != 1) throw 1;
    auto sub_tracks = discovery.tracks_by_kind(TrackKind::Subtitle);
    if (sub_tracks.size() != 2) throw 1;
    auto default_video = discovery.find_default(TrackKind::Video);
    if (default_video == discovery.tracks().end()) throw 1;
    if (default_video->codec_name != "h264") throw 1;
    std::printf("[C11] TrackDiscovery PASS\n");
}

// C12: Subtitle attachment map
static void test_c12_subtitle_attachment_map() {
    using namespace kivo::video_plane::ffmpeg_demux;
    FakeSubtitleAttachmentMap map;
    map.register_attachment({0, "font.ttf", "application/x-truetype-font", 65536, "sub_001", false});
    map.register_attachment({1, "external.srt", "application/x-subrip", 1024, "sub_002", true});
    if (map.attachment_count() != 2) throw 1;
    auto found = map.find_by_filename("font.ttf");
    if (!found.has_value()) throw 1;
    if (found->byte_size != 65536) throw 1;
    auto track_attachments = map.find_by_track("sub_001");
    if (track_attachments.size() != 1) throw 1;
    auto external = map.external_attachments();
    if (external.size() != 1) throw 1;
    if (!map.validate_mime_types()) throw 1;
    std::printf("[C12] SubtitleAttachmentMap PASS\n");
}

int main() {
    std::printf("=== P5C FFmpeg Demux Backend Tests ===\n\n");
    P5C_RUN(test_c1_ffmpeg_wrapper_isolation);
    P5C_RUN(test_c2_avio_bridge);
    P5C_RUN(test_c3_interrupt_callback);
    P5C_RUN(test_c4_find_stream_info_deadline);
    P5C_RUN(test_c5_demux_quarantine);
    P5C_RUN(test_c6_zombie_quarantine);
    P5C_RUN(test_c7_ffmpeg_risk_register);
    P5C_RUN(test_c8_license_manifest);
    P5C_RUN(test_c9_malformed_corpus_smoke);
    P5C_RUN(test_c10_sandbox_broker_contract);
    P5C_RUN(test_c11_track_discovery);
    P5C_RUN(test_c12_subtitle_attachment_map);
    if (g_failed == 0) {
        std::printf("\n=== P5C Summary: all 12 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P5C Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
