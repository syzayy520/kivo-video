#pragma once

#include "kivo/cinema_engine/source_core/emby_session_identity.hpp"
#include "kivo/cinema_engine/source_core/emby_session_runtime.hpp"
#include "kivo/cinema_engine/source_core/emby_session_failure.hpp"

namespace kivo::cinema_engine {

// Fake Emby session service for P2-036 testing.
class FakeEmbySessionService {
public:
    // Identity builders
    static EmbySessionIdentity build_movie_session_identity();
    static EmbySessionIdentity build_episode_session_identity();

    // Runtime builders
    static EmbySessionRuntime build_created_session();
    static EmbySessionRuntime build_playing_session();
    static EmbySessionRuntime build_paused_session();
    static EmbySessionRuntime build_torn_down_session();

    // Failure builders
    static EmbySessionFailure build_session_expired_failure();
    static EmbySessionFailure build_server_unreachable_failure();
    static EmbySessionFailure build_item_not_found_failure();
    static EmbySessionFailure build_access_denied_failure();
};

}  // namespace kivo::cinema_engine
