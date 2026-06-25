#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"

using namespace kivo::cinema_engine;

// ─── Test: default construction ───
static void test_default_construction() {
    RealSoftwareDecodeRuntime decoder;
    assert(!decoder.is_open());
    assert(!decoder.is_flushed());
    assert(decoder.codec_name().empty());
    assert(decoder.width() == 0);
    assert(decoder.height() == 0);
    assert(decoder.pixel_format().empty());
    assert(decoder.error_code().empty());
    assert(decoder.inspector_hint().empty());
    std::cout << "  PASS: default_construction\n";
}

// ─── Test: open nonexistent codec ───
static void test_open_nonexistent_codec() {
    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.open("nonexistent_codec_xyz", "trace_decode_001");
    assert(!ok);
    assert(!decoder.is_open());
    assert(!decoder.error_code().empty());
    std::cout << "  PASS: open_nonexistent_codec\n";
}

// ─── Test: decode when not open ───
static void test_decode_not_open() {
    RealSoftwareDecodeRuntime decoder;
    KivoPacket packet;
    auto result = decoder.decode(packet, "trace_decode_002");
    assert(result.success == false);
    assert(!result.error_message.empty());
    assert(result.metadata.trace_id == "trace_decode_002");
    std::cout << "  PASS: decode_not_open\n";
}

// ─── Test: flush when not open ───
static void test_flush_not_open() {
    RealSoftwareDecodeRuntime decoder;
    auto result = decoder.flush("trace_decode_003");
    assert(result.success == false);
    assert(!result.error_message.empty());
    std::cout << "  PASS: flush_not_open\n";
}

// ─── Test: close when not open ───
static void test_close_when_not_open() {
    RealSoftwareDecodeRuntime decoder;
    decoder.close();  // Should not crash
    assert(!decoder.is_open());
    std::cout << "  PASS: close_when_not_open\n";
}

// ─── Test: move construction ───
static void test_move_construction() {
    RealSoftwareDecodeRuntime dec1;
    RealSoftwareDecodeRuntime dec2(std::move(dec1));
    assert(!dec2.is_open());
    std::cout << "  PASS: move_construction\n";
}

// ─── Test: move assignment ───
static void test_move_assignment() {
    RealSoftwareDecodeRuntime dec1;
    RealSoftwareDecodeRuntime dec2;
    dec2 = std::move(dec1);
    assert(!dec2.is_open());
    std::cout << "  PASS: move_assignment\n";
}

// ─── Test: open h264 codec (software) ───
static void test_open_h264() {
    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.open("h264", "trace_h264");
    if (ok) {
        assert(decoder.is_open());
        assert(decoder.codec_name() == "h264");
        decoder.close();
        assert(!decoder.is_open());
    } else {
        assert(!decoder.error_code().empty());
    }
    std::cout << "  PASS: open_h264 (" << (ok ? "success" : decoder.error_code()) << ")\n";
}

// ─── Test: open aac codec ───
static void test_open_aac() {
    if (!RealProbeRuntime::is_ffmpeg_available()) {
        std::cout << "  SKIP: open_aac (ffmpeg disabled)\n";
        return;
    }

    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.open("aac", "trace_aac");
    if (ok) {
        assert(decoder.is_open());
        assert(decoder.codec_name() == "aac");
        decoder.close();
    }
    std::cout << "  PASS: open_aac (" << (ok ? "success" : "codec_not_found") << ")\n";
}

// ─── Test: decode result structure ───
static void test_decode_result_structure() {
    DecodeResult result;
    assert(result.success == false);
    assert(result.eof == false);
    assert(result.needs_more_input == false);
    assert(result.error_message.empty());
    assert(result.frame.frame_id.empty());
    assert(result.frame.stream_kind.empty());
    std::cout << "  PASS: decode_result_structure\n";
}

// ─── Test: KivoPacket structure ───
static void test_kivo_packet_structure() {
    KivoPacket packet;
    assert(packet.stream_index == -1);
    assert(packet.pts == 0);
    assert(packet.dts == 0);
    assert(packet.duration == 0);
    assert(packet.is_key_frame == false);
    assert(packet.is_corrupt == false);
    assert(packet.data_size == 0);
    assert(packet.data_ref.empty());
    std::cout << "  PASS: kivo_packet_structure\n";
}

// ─── Test: open then close lifecycle ───
static void test_open_close_lifecycle() {
    RealSoftwareDecodeRuntime decoder;
    bool ok = decoder.open("h264", "trace_lifecycle");
    if (ok) {
        assert(decoder.is_open());
        assert(!decoder.is_flushed());

        auto flush_result = decoder.flush("trace_flush");
        assert(decoder.is_flushed() || flush_result.needs_more_input || !flush_result.success);

        decoder.close();
        assert(!decoder.is_open());
    }
    std::cout << "  PASS: open_close_lifecycle\n";
}

int main() {
    std::cout << "real_software_decode_runtime_test:\n";
    test_default_construction();
    test_open_nonexistent_codec();
    test_decode_not_open();
    test_flush_not_open();
    test_close_when_not_open();
    test_move_construction();
    test_move_assignment();
    test_open_h264();
    test_open_aac();
    test_decode_result_structure();
    test_kivo_packet_structure();
    test_open_close_lifecycle();
    std::cout << "ALL 12 TESTS PASSED\n";
    return 0;
}
