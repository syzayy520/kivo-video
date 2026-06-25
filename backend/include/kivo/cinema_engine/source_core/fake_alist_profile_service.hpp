#pragma once

#include "kivo/cinema_engine/source_core/alist_server_profile.hpp"
#include "kivo/cinema_engine/source_core/alist_api_behavior.hpp"
#include "kivo/cinema_engine/source_core/alist_server_failure.hpp"

namespace kivo::cinema_engine {

// Fake AList profile service for P2-034 testing.
class FakeAListProfileService {
public:
    // Server profile builders
    static AListServerProfile build_standard_profile();
    static AListServerProfile build_webdav_enabled_profile();
    static AListServerProfile build_restricted_profile();

    // API behavior builders
    static AListApiBehavior build_full_api_behavior();
    static AListApiBehavior build_readonly_api_behavior();
    static AListApiBehavior build_minimal_api_behavior();

    // Failure builders
    static AListServerFailure build_auth_expired_failure();
    static AListServerFailure build_rate_limited_failure();
    static AListServerFailure build_backend_unavailable_failure();
    static AListServerFailure build_permission_denied_failure();
    static AListServerFailure build_not_found_failure();
};

}  // namespace kivo::cinema_engine
