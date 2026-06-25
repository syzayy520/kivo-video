#include "kivo/cinema_engine/source_core/fake_emby_session_service.hpp"

namespace kivo::cinema_engine {

// --- Identity builders ---

EmbySessionIdentity FakeEmbySessionService::build_movie_session_identity() {
    EmbySessionIdentity id;
    id.metadata.schema_version = "1.1";
    id.session_id = "emby-session-001";
    id.server_id = "emby-server-001";
    id.item_id = "emby-item-movie-42";
    id.media_source_id = "emby-media-src-001";
    id.user_id = "emby-user-alice";
    id.device_id = "device-win-pc-001";
    return id;
}

EmbySessionIdentity FakeEmbySessionService::build_episode_session_identity() {
    EmbySessionIdentity id;
    id.metadata.schema_version = "1.1";
    id.session_id = "emby-session-002";
    id.server_id = "emby-server-001";
    id.item_id = "emby-item-episode-99";
    id.media_source_id = "emby-media-src-002";
    id.user_id = "emby-user-bob";
    id.device_id = "device-android-001";
    return id;
}

// --- Runtime builders ---

EmbySessionRuntime FakeEmbySessionService::build_created_session() {
    EmbySessionRuntime r;
    r.metadata.schema_version = "1.1";
    r.session_id = "emby-session-001";
    r.state = EmbySessionState::Created;
    r.created_at = 1719200000;
    r.last_activity_at = 1719200000;
    r.playback_position_ticks = 0;
    r.is_paused = false;
    return r;
}

EmbySessionRuntime FakeEmbySessionService::build_playing_session() {
    EmbySessionRuntime r;
    r.metadata.schema_version = "1.1";
    r.session_id = "emby-session-001";
    r.state = EmbySessionState::Playing;
    r.created_at = 1719200000;
    r.last_activity_at = 1719203600;  // 1 hour later
    r.playback_position_ticks = 36000000000;  // 1 hour in ticks
    r.is_paused = false;
    return r;
}

EmbySessionRuntime FakeEmbySessionService::build_paused_session() {
    EmbySessionRuntime r;
    r.metadata.schema_version = "1.1";
    r.session_id = "emby-session-001";
    r.state = EmbySessionState::Paused;
    r.created_at = 1719200000;
    r.last_activity_at = 1719205400;  // 1.5 hours later
    r.playback_position_ticks = 36000000000;  // paused at 1 hour
    r.is_paused = true;
    return r;
}

EmbySessionRuntime FakeEmbySessionService::build_torn_down_session() {
    EmbySessionRuntime r;
    r.metadata.schema_version = "1.1";
    r.session_id = "emby-session-001";
    r.state = EmbySessionState::TornDown;
    r.created_at = 1719200000;
    r.last_activity_at = 1719207200;  // 2 hours later
    r.playback_position_ticks = 72000000000;  // 2 hours in ticks
    r.is_paused = false;
    return r;
}

// --- Failure builders ---

EmbySessionFailure FakeEmbySessionService::build_session_expired_failure() {
    EmbySessionFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbySessionFailureKind::SessionExpired;
    f.failure_reason = "session token has expired";
    f.retry_after_ms = 0;
    f.is_recoverable = true;
    return f;
}

EmbySessionFailure FakeEmbySessionService::build_server_unreachable_failure() {
    EmbySessionFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbySessionFailureKind::ServerUnreachable;
    f.failure_reason = "Emby server is unreachable";
    f.retry_after_ms = 30000;  // 30 seconds
    f.is_recoverable = true;
    return f;
}

EmbySessionFailure FakeEmbySessionService::build_item_not_found_failure() {
    EmbySessionFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbySessionFailureKind::ItemNotFound;
    f.failure_reason = "media item not found on server";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

EmbySessionFailure FakeEmbySessionService::build_access_denied_failure() {
    EmbySessionFailure f;
    f.metadata.schema_version = "1.1";
    f.failure_kind = EmbySessionFailureKind::AccessDenied;
    f.failure_reason = "user lacks permission to play this item";
    f.retry_after_ms = 0;
    f.is_recoverable = false;
    return f;
}

}  // namespace kivo::cinema_engine
