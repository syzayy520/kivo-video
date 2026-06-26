#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::video::source_core {

/// DirectPlay input carrier only. No final strategy logic.
/// Uses the central contract version constant.
/// Must not store raw input.
struct DirectPlayInput {
    std::uint32_t contract_version{kSourceCoreContractVersion};
    SourceIdentityId source_id;
    std::string container_format;
    CapabilityState seekable{CapabilityState::unknown};
    std::optional<std::uint64_t> content_length_bytes;
    std::optional<std::string> mime_type;

    // No decision logic. No raw URI. No platform objects.
};

}  // namespace kivo::video::source_core
