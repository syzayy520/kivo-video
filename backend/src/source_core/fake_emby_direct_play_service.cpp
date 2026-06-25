#include "kivo/cinema_engine/source_core/fake_emby_direct_play_service.hpp"

namespace kivo::cinema_engine {

// --- Request builders ---

EmbyDirectPlayRequest FakeEmbyDirectPlayService::build_standard_request() {
    EmbyDirectPlayRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dp-req-001";
    r.item_id = "emby-item-movie-42";
    r.media_source_id = "emby-media-src-001";
    r.device_profile_id = "device-profile-win-pc";
    r.max_streaming_bitrate = 0;  // unlimited
    return r;
}

EmbyDirectPlayRequest FakeEmbyDirectPlayService::build_low_bitrate_request() {
    EmbyDirectPlayRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dp-req-002";
    r.item_id = "emby-item-movie-42";
    r.media_source_id = "emby-media-src-001";
    r.device_profile_id = "device-profile-android";
    r.max_streaming_bitrate = 2000000;  // 2 Mbps
    return r;
}

// --- Response builders ---

EmbyDirectPlayResponse FakeEmbyDirectPlayService::build_direct_play_response() {
    EmbyDirectPlayResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dp-req-001";
    r.direct_play_url = "https://emby.example.com/Videos/42/stream?static=true";
    r.direct_play_container = "mp4";
    r.transcoding_url = "";
    r.transcoding_protocol = "";
    r.is_direct_play = true;
    return r;
}

EmbyDirectPlayResponse FakeEmbyDirectPlayService::build_transcode_fallback_response() {
    EmbyDirectPlayResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dp-req-002";
    r.direct_play_url = "";
    r.direct_play_container = "";
    r.transcoding_url = "https://emby.example.com/Videos/42/master.m3u8?MediaSourceId=001";
    r.transcoding_protocol = "hls";
    r.is_direct_play = false;
    return r;
}

EmbyDirectPlayResponse FakeEmbyDirectPlayService::build_hls_transcode_response() {
    EmbyDirectPlayResponse r;
    r.metadata.schema_version = "1.1";
    r.request_id = "dp-req-003";
    r.direct_play_url = "";
    r.direct_play_container = "";
    r.transcoding_url = "https://emby.example.com/Videos/42/live.m3u8";
    r.transcoding_protocol = "hls";
    r.is_direct_play = false;
    return r;
}

// --- Failure builders ---

EmbyDirectPlayFailure FakeEmbyDirectPlayService::build_item_not_found_failure() {
    EmbyDirectPlayFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbyDirectPlayFailureKind::ItemNotFound;
    f.failure_reason = "media item not found on Emby server";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

EmbyDirectPlayFailure FakeEmbyDirectPlayService::build_incompatible_format_failure() {
    EmbyDirectPlayFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbyDirectPlayFailureKind::IncompatibleFormat;
    f.failure_reason = "client device cannot play this format directly";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

EmbyDirectPlayFailure FakeEmbyDirectPlayService::build_server_error_failure() {
    EmbyDirectPlayFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbyDirectPlayFailureKind::ServerError;
    f.failure_reason = "Emby server internal error";
    f.retry_after_ms = 5000;  // 5 seconds
    f.is_recoverable = true;
    return f;
}

}  // namespace kivo::cinema_engine
