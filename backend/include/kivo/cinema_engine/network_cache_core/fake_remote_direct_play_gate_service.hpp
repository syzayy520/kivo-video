#pragma once

#include "kivo/cinema_engine/network_cache_core/remote_direct_play_gate_request.hpp"
#include "kivo/cinema_engine/network_cache_core/remote_direct_play_gate_result.hpp"
#include "kivo/cinema_engine/network_cache_core/remote_direct_play_gate_policy.hpp"

namespace kivo::cinema_engine {

// Fake remote direct play gate service for P2-038 testing.
class FakeRemoteDirectPlayGateService {
public:
    // Request builders
    static RemoteDirectPlayGateRequest build_direct_play_request();
    static RemoteDirectPlayGateRequest build_transcode_request();
    static RemoteDirectPlayGateRequest build_credential_request();
    static RemoteDirectPlayGateRequest build_redirect_leak_request();

    // Result builders
    static RemoteDirectPlayGateResult build_passed_result();
    static RemoteDirectPlayGateResult build_transcode_blocked_result();
    static RemoteDirectPlayGateResult build_credential_leak_result();

    // Policy builders
    static RemoteDirectPlayGatePolicy build_strict_policy();
    static RemoteDirectPlayGatePolicy build_permissive_policy();
};

}  // namespace kivo::cinema_engine
