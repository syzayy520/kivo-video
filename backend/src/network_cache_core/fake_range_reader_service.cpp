#include "kivo/cinema_engine/network_cache_core/fake_range_reader_service.hpp"

namespace kivo::cinema_engine {

NetworkRangeReader FakeRangeReaderService::build_range_reader() {
    NetworkRangeReader reader;
    reader.metadata.schema_version = "1.1";
    reader.reader_id = "range-reader-001";
    reader.adapter_id = "webdav-001";
    reader.base_url = "https://dav.example.com/media/";
    reader.supports_range = true;
    reader.max_range_size = 4194304;  // 4MB
    return reader;
}

RangeReadResult FakeRangeReaderService::build_successful_range_read() {
    RangeReadResult result;
    result.metadata.schema_version = "1.1";
    result.request_id = "range-req-001";
    result.success = true;
    result.bytes_read = 1048576;  // 1MB
    result.response_time_ms = 85;
    result.total_content_length = 5368709120;  // 5GB
    result.etag = "abc123def456";
    result.error_message = "";
    return result;
}

RangeReadResult FakeRangeReaderService::build_range_not_satisfiable() {
    RangeReadResult result;
    result.metadata.schema_version = "1.1";
    result.request_id = "range-req-002";
    result.success = false;
    result.bytes_read = 0;
    result.response_time_ms = 45;
    result.total_content_length = 0;
    result.etag = "";
    result.error_message = "HTTP 416 Range Not Satisfiable: requested range exceeds content length";
    return result;
}

RangeReadResult FakeRangeReaderService::build_partial_range_read() {
    RangeReadResult result;
    result.metadata.schema_version = "1.1";
    result.request_id = "range-req-003";
    result.success = true;
    result.bytes_read = 524288;  // 512KB partial
    result.response_time_ms = 120;
    result.total_content_length = 1073741824;  // 1GB
    result.etag = "partial-etag-789";
    result.error_message = "";
    return result;
}

RangeReadResult FakeRangeReaderService::build_range_read_error() {
    RangeReadResult result;
    result.metadata.schema_version = "1.1";
    result.request_id = "range-req-004";
    result.success = false;
    result.bytes_read = 0;
    result.response_time_ms = 15000;
    result.total_content_length = 0;
    result.etag = "";
    result.error_message = "Connection reset after 15s";
    return result;
}

}  // namespace kivo::cinema_engine
