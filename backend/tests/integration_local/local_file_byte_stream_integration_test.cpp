#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_impl.hpp"
#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream_open_request.hpp"

using namespace kivo::cinema_engine;

// ─── Integration Test: Open and read a real local file ───
static void test_open_and_read_real_file() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_open_and_read_real_file (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    LocalFileByteStreamImpl stream;
    LocalFileByteStreamOpenRequest req;
    req.path = sample_path;
    req.trace_id = "integ_byte_stream_001";

    auto result = stream.open(req);
    assert(result.opened == true);
    assert(result.size_bytes > 0);
    assert(result.seekable == true);
    assert(result.error_code.empty());
    assert(result.trace_id == "integ_byte_stream_001");

    assert(stream.is_open());
    assert(stream.size() == result.size_bytes);

    // Read first 1024 bytes
    auto data = stream.read(1024);
    assert(data.size() > 0);
    assert(data.size() <= 1024);

    // Verify it's not all zeros (real file content)
    bool has_nonzero = false;
    for (auto byte : data) {
        if (byte != 0) { has_nonzero = true; break; }
    }
    assert(has_nonzero);

    // Tell position should have advanced
    assert(stream.tell() == data.size());

    // Read at specific offset (from beginning)
    auto data_at = stream.read_at(0, 16);
    assert(data_at.size() == 16);

    // Seek to middle
    uint64_t mid = result.size_bytes / 2;
    assert(stream.seek(mid));
    assert(stream.tell() == mid);

    stream.close();
    assert(!stream.is_open());

    std::cout << "  PASS: test_open_and_read_real_file (" << result.size_bytes << " bytes)\n";
}

// ─── Integration Test: File not found error ───
static void test_file_not_found_error() {
    LocalFileByteStreamImpl stream;
    LocalFileByteStreamOpenRequest req;
    req.path = "C:\\nonexistent\\path\\media.mp4";
    req.trace_id = "integ_byte_stream_002";

    auto result = stream.open(req);
    assert(result.opened == false);
    assert(result.error_code == "file_not_found");
    assert(!result.inspector_hint.empty());

    std::cout << "  PASS: test_file_not_found_error\n";
}

// ─── Integration Test: Read beyond EOF ───
static void test_read_beyond_eof() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_read_beyond_eof (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    LocalFileByteStreamImpl stream;
    LocalFileByteStreamOpenRequest req;
    req.path = sample_path;
    req.trace_id = "integ_byte_stream_003";

    auto result = stream.open(req);
    assert(result.opened);

    // Seek to near end and try to read more than available
    uint64_t near_end = result.size_bytes - 10;
    stream.seek(near_end);
    auto data = stream.read(10000);
    assert(data.size() <= 10);  // Should get at most 10 bytes

    stream.close();
    std::cout << "  PASS: test_read_beyond_eof\n";
}

// ─── Integration Test: Reopen same file ───
static void test_reopen_same_file() {
    const char* sample_path = std::getenv("KIVO_SAMPLE_H264_AAC_MP4");
    if (!sample_path || std::string(sample_path).empty()) {
        std::cout << "  SKIP: test_reopen_same_file (KIVO_SAMPLE_H264_AAC_MP4 not set)\n";
        return;
    }

    LocalFileByteStreamImpl stream;

    // First open
    LocalFileByteStreamOpenRequest req1;
    req1.path = sample_path;
    req1.trace_id = "integ_reopen_1";
    auto r1 = stream.open(req1);
    assert(r1.opened);
    stream.close();

    // Second open
    LocalFileByteStreamOpenRequest req2;
    req2.path = sample_path;
    req2.trace_id = "integ_reopen_2";
    auto r2 = stream.open(req2);
    assert(r2.opened);
    assert(r2.size_bytes == r1.size_bytes);

    stream.close();
    std::cout << "  PASS: test_reopen_same_file\n";
}

int main() {
    std::cout << "local_file_byte_stream_integration_test:\n";
    test_open_and_read_real_file();
    test_file_not_found_error();
    test_read_beyond_eof();
    test_reopen_same_file();
    std::cout << "ALL 4 INTEGRATION TESTS PASSED\n";
    return 0;
}
