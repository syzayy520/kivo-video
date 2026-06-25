#pragma once

#include <cstdint>
#include <optional>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/source_core/webdav_server_profile.hpp"
#include "kivo/cinema_engine/source_core/webdav_range_behavior.hpp"
#include "kivo/cinema_engine/source_core/webdav_server_failure.hpp"

namespace kivo::cinema_engine {

// Outcome of profiling a WebDAV endpoint.
struct WebDavProbeResult {
    ContractMetadata metadata;
    std::string profile_id;
    WebDavServerProfile server_profile;
    WebDavRangeBehavior range_behavior;
    int64_t probe_latency_ms{0};
    bool probe_success{false};
    std::optional<WebDavServerFailure> failure;
};

}  // namespace kivo::cinema_engine
