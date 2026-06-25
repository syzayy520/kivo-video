#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/source_core/fake_emby_session_service.hpp"

using namespace kivo::cinema_engine;

// P2-036: Emby Session Runtime tests

void test_movie_session_identity() {
    auto id = FakeEmbySessionService::build_movie_session_identity();
    assert(id.metadata.schema_version == "1.1");
    assert(id.session_id == "emby-session-001");
    assert(id.server_id == "emby-server-001");
    assert(id.item_id == "emby-item-movie-42");
    assert(id.media_source_id == "emby-media-src-001");
    assert(id.user_id == "emby-user-alice");
    assert(id.device_id == "device-win-pc-001");
    std::cout << "PASS: test_movie_session_identity" << std::endl;
}

void test_episode_session_identity() {
    auto id = FakeEmbySessionService::build_episode_session_identity();
    assert(id.session_id == "emby-session-002");
    assert(id.item_id == "emby-item-episode-99");
    assert(id.user_id == "emby-user-bob");
    assert(id.device_id == "device-android-001");
    std::cout << "PASS: test_episode_session_identity" << std::endl;
}

void test_created_session() {
    auto r = FakeEmbySessionService::build_created_session();
    assert(r.session_id == "emby-session-001");
    assert(r.state == EmbySessionState::Created);
    assert(r.playback_position_ticks == 0);
    assert(r.is_paused == false);
    std::cout << "PASS: test_created_session" << std::endl;
}

void test_playing_session() {
    auto r = FakeEmbySessionService::build_playing_session();
    assert(r.state == EmbySessionState::Playing);
    assert(r.playback_position_ticks == 36000000000);  // 1 hour
    assert(r.is_paused == false);
    assert(r.last_activity_at > r.created_at);
    std::cout << "PASS: test_playing_session" << std::endl;
}

void test_paused_session() {
    auto r = FakeEmbySessionService::build_paused_session();
    assert(r.state == EmbySessionState::Paused);
    assert(r.is_paused == true);
    assert(r.playback_position_ticks == 36000000000);
    std::cout << "PASS: test_paused_session" << std::endl;
}

void test_session_teardown() {
    auto r = FakeEmbySessionService::build_torn_down_session();
    assert(r.state == EmbySessionState::TornDown);
    assert(r.playback_position_ticks == 72000000000);  // 2 hours
    std::cout << "PASS: test_session_teardown" << std::endl;
}

void test_session_expired_failure() {
    auto f = FakeEmbySessionService::build_session_expired_failure();
    assert(f.failure_kind == EmbySessionFailureKind::SessionExpired);
    assert(f.is_recoverable == true);
    assert(f.retry_after_ms == 0);
    std::cout << "PASS: test_session_expired_failure" << std::endl;
}

void test_server_unreachable_failure() {
    auto f = FakeEmbySessionService::build_server_unreachable_failure();
    assert(f.failure_kind == EmbySessionFailureKind::ServerUnreachable);
    assert(f.retry_after_ms == 30000);
    assert(f.is_recoverable == true);
    std::cout << "PASS: test_server_unreachable_failure" << std::endl;
}

void test_item_not_found_failure() {
    auto f = FakeEmbySessionService::build_item_not_found_failure();
    assert(f.failure_kind == EmbySessionFailureKind::ItemNotFound);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_item_not_found_failure" << std::endl;
}

void test_access_denied_failure() {
    auto f = FakeEmbySessionService::build_access_denied_failure();
    assert(f.failure_kind == EmbySessionFailureKind::AccessDenied);
    assert(f.is_recoverable == false);
    std::cout << "PASS: test_access_denied_failure" << std::endl;
}

int main() {
    // Identity tests
    test_movie_session_identity();
    test_episode_session_identity();

    // Runtime lifecycle tests
    test_created_session();
    test_playing_session();
    test_paused_session();
    test_session_teardown();

    // Failure tests
    test_session_expired_failure();
    test_server_unreachable_failure();
    test_item_not_found_failure();
    test_access_denied_failure();

    std::cout << "All P2-036 Emby Session Runtime tests passed." << std::endl;
    return 0;
}
