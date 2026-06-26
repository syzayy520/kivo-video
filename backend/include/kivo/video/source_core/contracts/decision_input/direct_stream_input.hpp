#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::video::source_core {

/// DirectStream input carrier only. No final strategy logic.
/// Uses the central contract version constant.
/// Must not store raw input.
struct DirectStreamInput {
    std::uint32_t contract_version{kSourceCoreContractVersion};
    SourceIdentityId source_id;
    std::optional<std::string> transcode_hint;
    std::optional<std::string> target_format_hint;
    std::optional<std::uint64_t> bandwidth_estimate_bps;

    // No decision logic. No raw URI. No platform objects.
};

}  // namespace kivo::video::source_core
