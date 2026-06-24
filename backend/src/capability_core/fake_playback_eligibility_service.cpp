#include "kivo/cinema_engine/capability_core/fake_playback_eligibility_service.hpp"

#include <sstream>

namespace kivo::cinema_engine {

CapabilityJoinResult build_fake_playback_eligibility(
    const ContractMetadata& metadata,
    std::string static_capability_ref,
    std::string device_capability_ref,
    std::string session_capability_ref,
    std::vector<std::string> runtime_observation_refs,
    std::vector<std::string> conflicts,
    std::vector<std::string> missing_capabilities,
    double confidence) {
    
    CapabilityJoinResult result;
    result.metadata = metadata;
    
    std::ostringstream id_stream;
    id_stream << "capability-join-" << metadata.schema_version;
    result.join_result_id = id_stream.str();
    
    result.static_capability_ref = std::move(static_capability_ref);
    result.device_capability_ref = std::move(device_capability_ref);
    result.session_capability_ref = std::move(session_capability_ref);
    result.runtime_observation_refs = std::move(runtime_observation_refs);
    result.conflicts = std::move(conflicts);
    result.missing_capabilities = std::move(missing_capabilities);
    result.confidence = confidence;
    result.expiry = "2026-12-31T23:59:59Z";
    
    return result;
}

}  // namespace kivo::cinema_engine
