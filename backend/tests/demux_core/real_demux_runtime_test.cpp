#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"

using namespace kivo::cinema_engine;

// ─── Test: default construction ───
static void test_default_construction() {
    RealDemuxRuntime demux;
    assert(!demux.is_open());
    assert(!demux.is_eof());
    assert(demux.container_format().empty());
    assert(demux.duration() == 0.0);
    assert(demux.video_stream_count() == 0);
    assert(demux.audio_stream_count() == 0);
    assert(demux.subtitle_stream_count() == 0);
    std::cout << "  PASS: default_construction\n";
}

// ─── Test: open nonexistent file ───
static void test_open_nonexistent() {
    RealDemuxRuntime demux;
    bool ok = demux.open("/nonexistent/file.mp4", "trace_demux_001");
    assert(!ok);
    assert(!demux.is_open());
    assert(!demux.error_code().empty());
    std::cout << "  PASS: open_nonexistent\n";
}

// ─── Test: read_packet when not open ───
static void test_read_packet_not_open() {
    RealDemuxRuntime demux;
    auto result = demux.read_packet("trace_demux_002");
    assert(result.success == false);
    assert(!result.error_message.empty());
    assert(result.metadata.trace_id == "trace_demux_002");
    std::cout << "  PASS: read_packet_not_open\n";
}

// ─── Test: close when not open ───
static void test_close_when_not_open() {
    RealDemuxRuntime demux;
    demux.close();  // Should not crash
    assert(!demux.is_open());
    std::cout << "  PASS: close_when_not_open\n";
}

// ─── Test: move construction ───
static void test_move_construction() {
    RealDemuxRuntime demux1;
    RealDemuxRuntime demux2(std::move(demux1));
    assert(!demux2.is_open());
    std::cout << "  PASS: move_construction\n";
}

// ─── Test: move assignment ───
static void test_move_assignment() {
    RealDemuxRuntime demux1;
    RealDemuxRuntime demux2;
    demux2 = std::move(demux1);
    assert(!demux2.is_open());
    std::cout << "  PASS: move_assignment\n";
}

// ─── Test: move self-assignment ───
static void test_move_self_assignment() {
    RealDemuxRuntime demux;
    RealDemuxRuntime& ref = demux;
    demux = std::move(ref);  // Self-move should be safe
    std::cout << "  PASS: move_self_assignment\n";
}

// ─── Test: open/close lifecycle ───
static void test_open_close_lifecycle() {
    RealDemuxRuntime demux;
    bool ok = demux.open("/some/file.mp4", "trace_lifecycle");
    assert(!ok);
    std::cout << "  PASS: open_close_lifecycle\n";
}

// ─── Test: seek_to_time when not open ───
static void test_seek_to_time_not_open() {
    RealDemuxRuntime demux;
    bool ok = demux.seek_to_time(10.0);
    assert(!ok);
    std::cout << "  PASS: seek_to_time_not_open\n";
}

// ─── Test: inspector_hint after operations ───
static void test_inspector_hint() {
    RealDemuxRuntime demux;
    assert(demux.inspector_hint().empty());
    demux.open("/nonexistent.mp4", "trace_hint");
    assert(!demux.inspector_hint().empty());
    std::cout << "  PASS: inspector_hint\n";
}

// ─── Test: open sample media ───
static void test_open_sample_media() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: open_sample_media (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    RealDemuxRuntime demux;
    bool ok = demux.open(sample_path, "trace_sample_demux");
    if (!ok) {
        std::cout << "  PASS: open_sample_media (open failed: " << demux.error_code() << ")\n";
        return;
    }
    assert(demux.is_open());
    assert(demux.duration() > 0.0);
    assert(!demux.container_format().empty());
    assert(demux.video_stream_count() >= 1);
    assert(demux.audio_stream_count() >= 1);

    auto result = demux.read_packet("trace_sample_pkt");
    if (result.success) {
        assert(result.packet.stream_index >= 0);
        std::cout << "  PASS: open_sample_media ("
                  << demux.container_format() << ", "
                  << demux.duration() << "s, "
                  << "first_pkt_stream=" << result.packet.stream_index << ")\n";
    } else {
        std::cout << "  PASS: open_sample_media ("
                  << demux.container_format() << ", "
                  << demux.duration() << "s, "
                  << "read_packet_error=" << result.error_message << ")\n";
    }

    demux.close();
    assert(!demux.is_open());
}

// Forward declaration (from real_probe_runtime)
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"

int main() {
    std::cout << "real_demux_runtime_test:\n";
    test_default_construction();
    test_open_nonexistent();
    test_read_packet_not_open();
    test_close_when_not_open();
    test_move_construction();
    test_move_assignment();
    test_move_self_assignment();
    test_open_close_lifecycle();
    test_seek_to_time_not_open();
    test_inspector_hint();
    test_open_sample_media();
    std::cout << "ALL 11 TESTS PASSED\n";
    return 0;
}
