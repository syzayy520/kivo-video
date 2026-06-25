#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_emby_direct_play_service.hpp"

using namespace kivo::cinema_engine;

// P2-037: Emby Direct Play Playback tests

void test_standard_request() {
    auto r = FakeEmbyDirectPlayService::build_standard_request();
    assert(r.metadata.schema_version == "1.1");
    assert(r.request_id == "dp-req-001");
    assert(r.item_id == "emby-item-movie-42");
    assert(r.media_source_id == "emby-media-src-001");
    assert(r.max_streaming_bitrate == 0);
    std::cout << "PASS: test_standard_request" << std::endl;
}

void test_low_bitrate_request() {
    auto r = FakeEmbyDirectPlayService::build_low_bitrate_request();
    assert(r.request_id == "dp-req-002");
    assert(r.max_streaming_bitrate == 2000000);
    assert(r.device_profile_id == "device-profile-android");
    std::cout << "PASS: test_low_bitrate_request" << std::endl;
}

void test_direct_play_response() {
    auto r = FakeEmbyDirectPlayService::build_direct_play_response();
    assert(r.request_id == "dp-req-001");
    assert(!r.direct_play_url.empty());
    assert(r.direct_play_container == "mp4");
    assert(r.is_direct_play == true);
    assert(r.transcoding_url.empty());
    std::cout << "PASS: test_direct_play_response" << std::endl;
}

void test_transcode_fallback_response() {
    auto r = FakeEmbyDirectPlayService::build_transcode_fallback_response();
    assert(r.request_id == "dp-req-002");
    assert(r.direct_play_url.empty());
    assert(!r.transcoding_url.empty());
    assert(r.transcoding_protocol == "hls");
    assert(r.is_direct_play == false);
    std::cout << "PASS: test_transcode_fallback_response" << std::endl;
}

void test_hls_transcode_response() {
    auto r = FakeEmbyDirectPlayService::build_hls_transcode_response();
    assert(r.transcoding_protocol == "hls");
    assert(r.is_direct_play == false);
    std::cout << "PASS: test_hls_transcode_response" << std::endl;
}

void test_request_response_correlation() {
    auto req = FakeEmbyDirectPlayService::build_standard_request();
    auto resp = FakeEmbyDirectPlayService::build_direct_play_response();
    assert(req.request_id == resp.request_id);
    std::cout << "PASS: test_request_response_correlation" << std::endl;
}

void test_item_not_found_failure() {
    auto f = FakeEmbyDirectPlayService::build_item_not_found_failure();
    assert(f.failure_kind == EmbyDirectPlayFailureKind::ItemNotFound);
    assert(f.is_recoverable == false);
    assert(f.retry_after_ms == 0);
    std::cout << "PASS: test_item_not_found_failure" << std::endl;
}

void test_incompatible_format_failure() {
    auto f = FakeEmbyDirectPlayService::build_incompatible_format_failure();
    assert(f.failure_kind == EmbyDirectPlayFailureKind::IncompatibleFormat);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_incompatible_format_failure" << std::endl;
}

void test_server_error_failure() {
    auto f = FakeEmbyDirectPlayService::build_server_error_failure();
    assert(f.failure_kind == EmbyDirectPlayFailureKind::ServerError);
    assert(f.retry_after_ms == 5000);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_server_error_failure" << std::endl;
}

void test_direct_play_eligibility() {
    // Direct play is possible when direct_play_url is non-empty
    auto direct = FakeEmbyDirectPlayService::build_direct_play_response();
    auto transcode = FakeEmbyDirectPlayService::build_transcode_fallback_response();
    assert(direct.is_direct_play == true);
    assert(transcode.is_direct_play == false);
    std::cout << "PASS: test_direct_play_eligibility" << std::endl;
}

int main() {
    // Request tests
    test_standard_request();
    test_low_bitrate_request();

    // Response tests
    test_direct_play_response();
    test_transcode_fallback_response();
    test_hls_transcode_response();
    test_request_response_correlation();

    // Failure tests
    test_item_not_found_failure();
    test_incompatible_format_failure();
    test_server_error_failure();

    // Eligibility test
    test_direct_play_eligibility();

    std::cout << "All P2-037 Emby Direct Play Playback tests passed." << std::endl;
    return 0;
}
