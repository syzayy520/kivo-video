#pragma once

#include "kivo/cinema_engine/source_core/emby_direct_play_request.hpp"
#include "kivo/cinema_engine/source_core/emby_direct_play_response.hpp"
#include "kivo/cinema_engine/source_core/emby_direct_play_failure.hpp"

namespace kivo::cinema_engine {

// Fake Emby direct play service for P2-037 testing.
class FakeEmbyDirectPlayService {
public:
    // Request builders
    static EmbyDirectPlayRequest build_standard_request();
    static EmbyDirectPlayRequest build_low_bitrate_request();

    // Response builders
    static EmbyDirectPlayResponse build_direct_play_response();
    static EmbyDirectPlayResponse build_transcode_fallback_response();
    static EmbyDirectPlayResponse build_hls_transcode_response();

    // Failure builders
    static EmbyDirectPlayFailure build_item_not_found_failure();
    static EmbyDirectPlayFailure build_incompatible_format_failure();
    static EmbyDirectPlayFailure build_server_error_failure();
};

}  // namespace kivo::cinema_engine
