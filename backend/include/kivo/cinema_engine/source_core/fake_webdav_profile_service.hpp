#pragma once

#include "kivo/cinema_engine/source_core/webdav_server_profile.hpp"
#include "kivo/cinema_engine/source_core/webdav_range_behavior.hpp"
#include "kivo/cinema_engine/source_core/webdav_server_failure.hpp"
#include "kivo/cinema_engine/source_core/webdav_probe_result.hpp"

namespace kivo::cinema_engine {

// Fake WebDAV profile service for P2-032 testing.
class FakeWebDavProfileService {
public:
    // Server profile builders
    static WebDavServerProfile build_nginx_profile();
    static WebDavServerProfile build_apache_profile();
    static WebDavServerProfile build_rclone_profile();
    static WebDavServerProfile build_synology_profile();
    static WebDavServerProfile build_no_range_profile();

    // Range behavior builders
    static WebDavRangeBehavior build_nginx_range_behavior();
    static WebDavRangeBehavior build_partial_range_behavior();
    static WebDavRangeBehavior build_no_range_behavior();

    // Failure builders
    static WebDavServerFailure build_connection_failure();
    static WebDavServerFailure build_auth_failure();
    static WebDavServerFailure build_range_not_supported_failure();

    // Probe result builders
    static WebDavProbeResult build_successful_probe();
    static WebDavProbeResult build_failed_probe();
};

}  // namespace kivo::cinema_engine
