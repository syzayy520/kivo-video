#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/decode_core/fake_frame_queue_service.hpp"

using namespace kivo::cinema_engine;

static void test_default_config() {
    auto config = build_default_frame_queue_config("video-queue");
    assert(config.metadata.schema_version == "1.1");
    assert(config.queue_id == "video-queue");
    assert(config.max_frames == 10);
    assert(config.max_bytes == 100 * 1024 * 1024);
    assert(config.drop_on_overflow == false);
    std::cout << "  PASS: default_config\n";
}

static void test_custom_config() {
    auto config = build_frame_queue_config("audio-queue", 20, 50 * 1024 * 1024);
    assert(config.queue_id == "audio-queue");
    assert(config.max_frames == 20);
    assert(config.max_bytes == 50 * 1024 * 1024);
    std::cout << "  PASS: custom_config\n";
}

static void test_empty_status() {
    auto status = build_empty_frame_queue_status("test-queue");
    assert(status.queue_id == "test-queue");
    assert(status.current_frames == 0);
    assert(status.current_bytes == 0);
    assert(status.is_full == false);
    assert(status.is_empty == true);
    std::cout << "  PASS: empty_status\n";
}

static void test_full_status() {
    auto status = build_full_frame_queue_status("test-queue");
    assert(status.queue_id == "test-queue");
    assert(status.current_frames == 10);
    assert(status.current_bytes == 100 * 1024 * 1024);
    assert(status.is_full == true);
    assert(status.is_empty == false);
    std::cout << "  PASS: full_status\n";
}

static void test_partial_status() {
    auto status = build_frame_queue_status("test-queue", 5, 50 * 1024 * 1024);
    assert(status.queue_id == "test-queue");
    assert(status.current_frames == 5);
    assert(status.current_bytes == 50 * 1024 * 1024);
    assert(status.is_full == false);
    assert(status.is_empty == false);
    std::cout << "  PASS: partial_status\n";
}

int main() {
    std::cout << "frame_queue_test:\n";
    test_default_config();
    test_custom_config();
    test_empty_status();
    test_full_status();
    test_partial_status();
    std::cout << "ALL 5 TESTS PASSED\n";
    return 0;
}