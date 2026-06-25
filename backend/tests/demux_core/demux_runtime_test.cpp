#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/demux_core/fake_demux_runtime.hpp"

using namespace kivo::cinema_engine;

static void test_successful_demux() {
    auto result = build_successful_demux_result();
    assert(result.success == true);
    assert(result.eof == false);
    assert(result.error_message.empty());
    assert(result.packet.packet_id == "packet-001");
    assert(result.packet.stream_index == 0);
    assert(result.packet.pts == 1000);
    assert(result.packet.dts == 1000);
    assert(result.packet.duration == 40);
    assert(result.packet.is_key_frame == true);
    assert(result.packet.is_corrupt == false);
    assert(static_cast<int64_t>(result.packet.data.size()) == 1024);
    std::cout << "  PASS: successful_demux\n";
}

static void test_eof_demux() {
    auto result = build_eof_demux_result();
    assert(result.success == false);
    assert(result.eof == true);
    assert(result.error_message == "End of file");
    assert(result.packet.packet_id.empty());
    std::cout << "  PASS: eof_demux\n";
}

static void test_failed_demux() {
    auto result = build_failed_demux_result("Invalid format");
    assert(result.success == false);
    assert(result.eof == false);
    assert(result.error_message == "Invalid format");
    std::cout << "  PASS: failed_demux\n";
}

static void test_custom_packet() {
    KivoPacket packet;
    packet.metadata.schema_version = "1.1";
    packet.packet_id = "custom-packet";
    packet.stream_index = 1;
    packet.pts = 2000;
    packet.dts = 2000;
    packet.duration = 20;
    packet.is_key_frame = false;
    packet.is_corrupt = false;
    packet.data.resize(512, 0xAB);
    auto result = build_demux_result_with_packet(packet);
    assert(result.success == true);
    assert(result.packet.packet_id == "custom-packet");
    assert(result.packet.stream_index == 1);
    assert(result.packet.pts == 2000);
    std::cout << "  PASS: custom_packet\n";
}

int main() {
    std::cout << "demux_runtime_test:\n";
    test_successful_demux();
    test_eof_demux();
    test_failed_demux();
    test_custom_packet();
    std::cout << "ALL 4 TESTS PASSED\n";
    return 0;
}