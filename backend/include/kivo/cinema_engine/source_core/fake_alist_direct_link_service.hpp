#pragma once

#include "kivo/cinema_engine/source_core/alist_direct_link_request.hpp"
#include "kivo/cinema_engine/source_core/alist_direct_link_response.hpp"
#include "kivo/cinema_engine/source_core/alist_direct_link_identity.hpp"
#include "kivo/cinema_engine/source_core/alist_direct_link_failure.hpp"

namespace kivo::cinema_engine {

// Fake AList direct link service for P2-035 testing.
class FakeAListDirectLinkService {
public:
    // Request builders
    static AListDirectLinkRequest build_simple_request();
    static AListDirectLinkRequest build_custom_ttl_request();

    // Response builders
    static AListDirectLinkResponse build_successful_link_response();
    static AListDirectLinkResponse build_short_ttl_response();
    static AListDirectLinkResponse build_no_ttl_response();

    // Identity builders
    static AListDirectLinkIdentity build_initial_identity();
    static AListDirectLinkIdentity build_refreshed_identity();
    static AListDirectLinkIdentity build_multi_refresh_identity();

    // Failure builders
    static AListDirectLinkFailure build_link_expired_failure();
    static AListDirectLinkFailure build_server_unreachable_failure();
    static AListDirectLinkFailure build_auth_denied_failure();
};

}  // namespace kivo::cinema_engine
