#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/source_core/fake_file_identity_service.hpp"

using namespace kivo::cinema_engine;

static void test_stable_identity() {
    auto id = build_stable_file_identity("C:\\Videos\\test.mp4");
    assert(id.file_path == "C:\\Videos\\test.mp4");
    assert(id.volume_serial == "VOL-001");
    assert(id.file_index == 12345);
    assert(id.file_size == 1024 * 1024);
    assert(id.identity_stable);
    std::cout << "  PASS: stable_identity\n";
}

static void test_unstable_identity() {
    auto id = build_unstable_file_identity("C:\\Temp\\temp.dat");
    assert(!id.identity_stable);
    assert(id.file_size == 0);
    std::cout << "  PASS: unstable_identity\n";
}

static void test_custom_identity() {
    auto id = build_file_identity_with_ids("D:\\Media\\movie.mkv", "VOL-002", 54321);
    assert(id.file_path == "D:\\Media\\movie.mkv");
    assert(id.volume_serial == "VOL-002");
    assert(id.file_index == 54321);
    assert(id.identity_stable);
    std::cout << "  PASS: custom_identity\n";
}

int main() {
    std::cout << "file_identity_test:\n";
    test_stable_identity();
    test_unstable_identity();
    test_custom_identity();
    std::cout << "ALL 3 TESTS PASSED\n";
    return 0;
}
