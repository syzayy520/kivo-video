#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/demux_core/fake_probe_service.hpp"

using namespace kivo::cinema_engine;

static void test_successful_probe() {
    auto result = build_successful_probe_result();
    assert(result.success == true);
    assert(result.error_message.empty());
    assert(result.inventory.container_format == "mp4");
    assert(result.inventory.video_streams.size() == 1);
    assert(result.inventory.audio_streams.size() == 1);
    assert(result.inventory.subtitle_streams.size() == 1);
    std::cout << "  PASS: successful_probe\n";
}

static void test_failed_probe() {
    auto result = build_failed_probe_result("File not found");
    assert(result.success == false);
    assert(result.error_message == "File not found");
    assert(result.inventory.container_format.empty());
    std::cout << "  PASS: failed_probe\n";
}

static void test_custom_streams() {
    std::vector<std::string> video = {"hevc:3840x2160@60fps", "h264:1920x1080@30fps"};
    std::vector<std::string> audio = {"aac:2ch:48000Hz", "ac3:6ch:48000Hz"};
    std::vector<std::string> subs = {"srt:eng", "srt:chi", "ass:jpn"};
    auto result = build_probe_result_with_streams("mkv", video, audio, subs);
    assert(result.success == true);
    assert(result.inventory.container_format == "mkv");
    assert(result.inventory.video_streams.size() == 2);
    assert(result.inventory.audio_streams.size() == 2);
    assert(result.inventory.subtitle_streams.size() == 3);
    std::cout << "  PASS: custom_streams\n";
}

int main() {
    std::cout << "probe_service_test:\n";
    test_successful_probe();
    test_failed_probe();
    test_custom_streams();
    std::cout << "ALL 3 TESTS PASSED\n";
    return 0;
}