#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_webdav_range_playback_service.hpp"

using namespace kivo::cinema_engine;

// P2-033: WebDAV Range Playback tests

void test_valid_range_request() {
    auto r = FakeWebDavRangePlaybackService::build_valid_range_request();
    assert(r.metadata.schema_version == "1.1");
    assert(r.request_id == "range-req-001");
    assert(r.byte_offset == 10485760);
    assert(r.byte_length == 10485760);
    assert(r.etag.empty());
    assert(r.if_range.empty());
    std::cout << "PASS: test_valid_range_request" << std::endl;
}

void test_zero_length_request() {
    auto r = FakeWebDavRangePlaybackService::build_zero_length_request();
    assert(r.byte_offset == 0);
    assert(r.byte_length == 0);
    std::cout << "PASS: test_zero_length_request" << std::endl;
}

void test_negative_offset_request() {
    auto r = FakeWebDavRangePlaybackService::build_negative_offset_request();
    assert(r.byte_offset == -1);
    assert(r.byte_length == 1024);
    std::cout << "PASS: test_negative_offset_request" << std::endl;
}

void test_conditional_request() {
    auto r = FakeWebDavRangePlaybackService::build_conditional_request();
    assert(r.etag == "\"abc123\"");
    assert(r.if_range == "\"abc123\"");
    assert(r.byte_offset == 0);
    assert(r.byte_length == 524288);
    std::cout << "PASS: test_conditional_request" << std::endl;
}

void test_partial_content_response() {
    auto r = FakeWebDavRangePlaybackService::build_partial_content_response();
    assert(r.status_code == 206);
    assert(r.content_range_total == 1073741824);
    assert(r.content_range_start == 10485760);
    assert(r.content_range_end == 20971519);
    assert(r.actual_byte_count == 10485760);
    assert(r.is_full_content == false);
    std::cout << "PASS: test_partial_content_response" << std::endl;
}

void test_full_content_response() {
    auto r = FakeWebDavRangePlaybackService::build_full_content_response();
    assert(r.status_code == 200);
    assert(r.is_full_content == true);
    assert(r.actual_byte_count == 1073741824);
    assert(r.content_range_total == 0);
    std::cout << "PASS: test_full_content_response" << std::endl;
}

void test_not_satisfiable_response() {
    auto r = FakeWebDavRangePlaybackService::build_not_satisfiable_response();
    assert(r.status_code == 416);
    assert(r.actual_byte_count == 0);
    assert(r.is_full_content == false);
    std::cout << "PASS: test_not_satisfiable_response" << std::endl;
}

void test_not_modified_response() {
    auto r = FakeWebDavRangePlaybackService::build_not_modified_response();
    assert(r.status_code == 304);
    assert(r.actual_byte_count == 0);
    assert(r.is_full_content == false);
    std::cout << "PASS: test_not_modified_response" << std::endl;
}

void test_full_download_fallback() {
    auto f = FakeWebDavRangePlaybackService::build_full_download_fallback();
    assert(f.fallback_strategy == WebDavFallbackStrategy::FullDownload);
    assert(f.estimated_download_size == 1073741824);
    assert(!f.user_visible_hint.empty());
    std::cout << "PASS: test_full_download_fallback" << std::endl;
}

void test_seek_to_zero_fallback() {
    auto f = FakeWebDavRangePlaybackService::build_seek_to_zero_fallback();
    assert(f.fallback_strategy == WebDavFallbackStrategy::SeekToZero);
    assert(f.estimated_download_size == 0);
    std::cout << "PASS: test_seek_to_zero_fallback" << std::endl;
}

void test_abort_fallback() {
    auto f = FakeWebDavRangePlaybackService::build_abort_fallback();
    assert(f.fallback_strategy == WebDavFallbackStrategy::Abort);
    assert(!f.user_visible_hint.empty());
    std::cout << "PASS: test_abort_fallback" << std::endl;
}

void test_request_response_binding() {
    auto req = FakeWebDavRangePlaybackService::build_valid_range_request();
    auto resp = FakeWebDavRangePlaybackService::build_partial_content_response();
    assert(req.request_id == resp.request_id);
    assert(resp.actual_byte_count == req.byte_length);
    std::cout << "PASS: test_request_response_binding" << std::endl;
}

void test_fallback_url_binding() {
    auto req = FakeWebDavRangePlaybackService::build_valid_range_request();
    auto fb = FakeWebDavRangePlaybackService::build_abort_fallback();
    assert(fb.original_request_url == req.url);
    std::cout << "PASS: test_fallback_url_binding" << std::endl;
}

int main() {
    // Request tests
    test_valid_range_request();
    test_zero_length_request();
    test_negative_offset_request();
    test_conditional_request();

    // Response tests
    test_partial_content_response();
    test_full_content_response();
    test_not_satisfiable_response();
    test_not_modified_response();

    // Fallback tests
    test_full_download_fallback();
    test_seek_to_zero_fallback();
    test_abort_fallback();

    // Cross-type binding tests
    test_request_response_binding();
    test_fallback_url_binding();

    std::cout << "All P2-033 WebDAV Range Playback tests passed." << std::endl;
    return 0;
}
