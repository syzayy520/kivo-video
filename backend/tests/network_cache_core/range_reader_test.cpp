#include "kivo/cinema_engine/network_cache_core/fake_range_reader_service.hpp"
#include <cassert>
#include <iostream>

using namespace kivo::cinema_engine;

void test_range_reader_identity() {
    auto reader = FakeRangeReaderService::build_range_reader();
    assert(reader.reader_id == "range-reader-001");
    assert(reader.adapter_id == "webdav-001");
    assert(reader.supports_range);
    assert(reader.max_range_size == 4194304);
    std::cout << "PASS: test_range_reader_identity" << std::endl;
}

void test_successful_range_read() {
    auto result = FakeRangeReaderService::build_successful_range_read();
    assert(result.request_id == "range-req-001");
    assert(result.success);
    assert(result.bytes_read == 1048576);
    assert(result.response_time_ms == 85);
    assert(result.total_content_length == 5368709120);
    assert(result.etag == "abc123def456");
    assert(result.error_message.empty());
    std::cout << "PASS: test_successful_range_read" << std::endl;
}

void test_range_not_satisfiable() {
    auto result = FakeRangeReaderService::build_range_not_satisfiable();
    assert(result.request_id == "range-req-002");
    assert(!result.success);
    assert(result.bytes_read == 0);
    assert(result.total_content_length == 0);
    assert(result.etag.empty());
    assert(!result.error_message.empty());
    std::cout << "PASS: test_range_not_satisfiable" << std::endl;
}

void test_partial_range_read() {
    auto result = FakeRangeReaderService::build_partial_range_read();
    assert(result.request_id == "range-req-003");
    assert(result.success);
    assert(result.bytes_read == 524288);
    assert(result.response_time_ms == 120);
    assert(result.total_content_length == 1073741824);
    assert(result.etag == "partial-etag-789");
    std::cout << "PASS: test_partial_range_read" << std::endl;
}

void test_range_read_error() {
    auto result = FakeRangeReaderService::build_range_read_error();
    assert(result.request_id == "range-req-004");
    assert(!result.success);
    assert(result.bytes_read == 0);
    assert(result.response_time_ms == 15000);
    assert(result.etag.empty());
    assert(result.error_message.find("Connection reset") != std::string::npos);
    std::cout << "PASS: test_range_read_error" << std::endl;
}

int main() {
    test_range_reader_identity();
    test_successful_range_read();
    test_range_not_satisfiable();
    test_partial_range_read();
    test_range_read_error();

    std::cout << "All network range reader tests passed!" << std::endl;
    return 0;
}
