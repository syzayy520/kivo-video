#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cstdint>
#include <string>

namespace kivo::video::source_core {

/// Skeleton produces contract-pass-only evidence.
/// Runtime-pass evidence must NOT be produced by this skeleton.
struct SourceEvidenceSnapshot {
    std::uint32_t contract_version{kSourceCoreContractVersion};
    SourceIdentityId source_id;
    ProviderKind kind{ProviderKind::unknown};
    std::string uri_redacted;  // display-safe, no raw input
    SourceCapabilitySnapshot capability;
    bool schema_validated{true};
    bool redaction_verified{true};

    // No raw input. No credentials. No platform objects.
};

}  // namespace kivo::video::source_core
