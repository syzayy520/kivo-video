#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/media_timeline_core/fake_timebase_service.hpp"

using namespace kivo::cinema_engine;

static void test_container_time_1_1000() {
    // 1/1000 timebase: 1 second = 1000 container ticks
    auto result = normalize_from_container_time(1, 1000, 1000);
    assert(result.is_valid);
    assert(result.kivo_media_time_us == 1000000);  // 1 second
    std::cout << "  PASS: container_time_1_1000\n";
}

static void test_container_time_1_90000() {
    // 1/90000 timebase (common for MPEG-TS): 90000 ticks = 1 second
    auto result = normalize_from_container_time(1, 90000, 90000);
    assert(result.is_valid);
    assert(result.kivo_media_time_us == 1000000);  // 1 second
    std::cout << "  PASS: container_time_1_90000\n";
}

static void test_stream_time_1_48000() {
    // Audio 48kHz: 48000 samples = 1 second
    auto result = normalize_from_stream_time(1, 48000, 48000);
    assert(result.is_valid);
    assert(result.kivo_media_time_us == 1000000);  // 1 second
    std::cout << "  PASS: stream_time_1_48000\n";
}

static void test_roundtrip_container() {
    auto normalized = normalize_from_container_time(1, 1000, 500);
    assert(normalized.is_valid);
    assert(normalized.kivo_media_time_us == 500000);  // 0.5 second

    int64_t back = kivo_media_time_to_container_time(500000, 1, 1000);
    assert(back == 500);
    std::cout << "  PASS: roundtrip_container\n";
}

static void test_invalid_timebase() {
    auto result = normalize_from_container_time(1, 0, 100);
    assert(!result.is_valid);
    std::cout << "  PASS: invalid_timebase\n";
}

static void test_zero_time() {
    auto result = normalize_from_container_time(1, 1000, 0);
    assert(result.is_valid);
    assert(result.kivo_media_time_us == 0);
    std::cout << "  PASS: zero_time\n";
}

int main() {
    std::cout << "timebase_normalization_test:\n";
    test_container_time_1_1000();
    test_container_time_1_90000();
    test_stream_time_1_48000();
    test_roundtrip_container();
    test_invalid_timebase();
    test_zero_time();
    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}
