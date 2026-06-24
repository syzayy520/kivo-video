#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/source_core/fake_local_file_source_service.hpp"

using namespace kivo::cinema_engine;

static void test_local_file_stream() {
    auto stream = build_local_file_stream("local-001", 1024 * 1024);
    assert(stream.metadata.schema_version == "1.1");
    assert(stream.stream_id == "local-001");
    assert(stream.source_kind == "local_file");
    assert(stream.position == 0);
    assert(stream.length == 1024 * 1024);
    assert(stream.is_seekable == true);
    assert(stream.is_readable == true);
    std::cout << "  PASS: local_file_stream\n";
}

static void test_network_range_stream() {
    auto stream = build_network_range_stream("net-001", 2048 * 1024);
    assert(stream.metadata.schema_version == "1.1");
    assert(stream.stream_id == "net-001");
    assert(stream.source_kind == "network_range");
    assert(stream.position == 0);
    assert(stream.length == 2048 * 1024);
    assert(stream.is_seekable == true);
    assert(stream.is_readable == true);
    std::cout << "  PASS: network_range_stream\n";
}

static void test_cache_segment_stream() {
    auto stream = build_cache_segment_stream("cache-001", 512 * 1024);
    assert(stream.metadata.schema_version == "1.1");
    assert(stream.stream_id == "cache-001");
    assert(stream.source_kind == "cache_segment");
    assert(stream.position == 0);
    assert(stream.length == 512 * 1024);
    assert(stream.is_seekable == true);
    assert(stream.is_readable == true);
    std::cout << "  PASS: cache_segment_stream\n";
}

int main() {
    std::cout << "local_file_source_test:\n";
    test_local_file_stream();
    test_network_range_stream();
    test_cache_segment_stream();
    std::cout << "ALL 3 TESTS PASSED\n";
    return 0;
}