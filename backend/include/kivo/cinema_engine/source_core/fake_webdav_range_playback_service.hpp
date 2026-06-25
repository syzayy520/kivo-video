#pragma once

#include "kivo/cinema_engine/source_core/webdav_range_request.hpp"
#include "kivo/cinema_engine/source_core/webdav_range_response.hpp"
#include "kivo/cinema_engine/source_core/webdav_range_fallback.hpp"

namespace kivo::cinema_engine {

// Fake WebDAV range playback service for P2-033 testing.
class FakeWebDavRangePlaybackService {
public:
    // Request builders
    static WebDavRangeRequest build_valid_range_request();
    static WebDavRangeRequest build_zero_length_request();
    static WebDavRangeRequest build_negative_offset_request();
    static WebDavRangeRequest build_conditional_request();

    // Response builders
    static WebDavRangeResponse build_partial_content_response();
    static WebDavRangeResponse build_full_content_response();
    static WebDavRangeResponse build_not_satisfiable_response();
    static WebDavRangeResponse build_not_modified_response();

    // Fallback builders
    static WebDavRangeFallback build_full_download_fallback();
    static WebDavRangeFallback build_seek_to_zero_fallback();
    static WebDavRangeFallback build_abort_fallback();
};

}  // namespace kivo::cinema_engine
