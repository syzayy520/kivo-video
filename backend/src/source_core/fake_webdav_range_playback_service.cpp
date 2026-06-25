#include "kivo/cinema_engine/source_core/fake_webdav_range_playback_service.hpp"

namespace kivo::cinema_engine {

WebDavRangeRequest FakeWebDavRangePlaybackService::build_valid_range_request() {
    WebDavRangeRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-001";
    r.url = "https://example.test/dav/media/movie.mkv";
    r.byte_offset = 10485760;     // 10 MB
    r.byte_length = 10485760;     // 10 MB
    return r;
}

WebDavRangeRequest FakeWebDavRangePlaybackService::build_zero_length_request() {
    WebDavRangeRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-002";
    r.url = "https://example.test/dav/media/movie.mkv";
    r.byte_offset = 0;
    r.byte_length = 0;
    return r;
}

WebDavRangeRequest FakeWebDavRangePlaybackService::build_negative_offset_request() {
    WebDavRangeRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-003";
    r.url = "https://example.test/dav/media/movie.mkv";
    r.byte_offset = -1;
    r.byte_length = 1024;
    return r;
}

WebDavRangeRequest FakeWebDavRangePlaybackService::build_conditional_request() {
    WebDavRangeRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-004";
    r.url = "https://example.test/dav/media/movie.mkv";
    r.byte_offset = 0;
    r.byte_length = 524288;       // 512 KB
    r.etag = "\"abc123\"";
    r.if_range = "\"abc123\"";
    return r;
}

WebDavRangeResponse FakeWebDavRangePlaybackService::build_partial_content_response() {
    WebDavRangeResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-001";
    r.status_code = 206;
    r.content_range_total = 1073741824;   // 1 GB total
    r.content_range_start = 10485760;     // 10 MB start
    r.content_range_end = 20971519;       // ~20 MB end
    r.actual_byte_count = 10485760;       // 10 MB received
    r.is_full_content = false;
    return r;
}

WebDavRangeResponse FakeWebDavRangePlaybackService::build_full_content_response() {
    WebDavRangeResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-002";
    r.status_code = 200;
    r.content_range_total = 0;
    r.content_range_start = 0;
    r.content_range_end = 0;
    r.actual_byte_count = 1073741824;     // 1 GB full body
    r.is_full_content = true;
    return r;
}

WebDavRangeResponse FakeWebDavRangePlaybackService::build_not_satisfiable_response() {
    WebDavRangeResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-003";
    r.status_code = 416;
    r.content_range_total = 1073741824;
    r.content_range_start = 0;
    r.content_range_end = 0;
    r.actual_byte_count = 0;
    r.is_full_content = false;
    return r;
}

WebDavRangeResponse FakeWebDavRangePlaybackService::build_not_modified_response() {
    WebDavRangeResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "range-req-004";
    r.status_code = 304;
    r.content_range_total = 1073741824;
    r.content_range_start = 0;
    r.content_range_end = 0;
    r.actual_byte_count = 0;
    r.is_full_content = false;
    return r;
}

WebDavRangeFallback FakeWebDavRangePlaybackService::build_full_download_fallback() {
    WebDavRangeFallback f;
    f.metadata.schema_version = "1.1";
    f.fallback_id = "fallback-001";
    f.original_request_url = "https://example.test/dav/media/movie.mkv";
    f.fallback_strategy = WebDavFallbackStrategy::FullDownload;
    f.estimated_download_size = 1073741824;  // 1 GB
    f.user_visible_hint = "Server does not support range requests. Downloading full file.";
    return f;
}

WebDavRangeFallback FakeWebDavRangePlaybackService::build_seek_to_zero_fallback() {
    WebDavRangeFallback f;
    f.metadata.schema_version = "1.1";
    f.fallback_id = "fallback-002";
    f.original_request_url = "https://example.test/dav/media/movie.mkv";
    f.fallback_strategy = WebDavFallbackStrategy::SeekToZero;
    f.estimated_download_size = 0;
    f.user_visible_hint = "Range request failed. Restarting playback from beginning.";
    return f;
}

WebDavRangeFallback FakeWebDavRangePlaybackService::build_abort_fallback() {
    WebDavRangeFallback f;
    f.metadata.schema_version = "1.1";
    f.fallback_id = "fallback-003";
    f.original_request_url = "https://example.test/dav/media/movie.mkv";
    f.fallback_strategy = WebDavFallbackStrategy::Abort;
    f.estimated_download_size = 0;
    f.user_visible_hint = "Cannot play this file from the server.";
    return f;
}

}  // namespace kivo::cinema_engine
