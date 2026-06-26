#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/uri/redacted_source_uri.hpp"
#include "kivo/video/source_core/runtime/session/opaque_source_access_ref.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"
#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace kivo::video::source_core {

struct DirectStreamInput {
    static constexpr std::uint32_t contract_schema_version = kSourceCoreContractVersion;

    SourceIdentity identity;
    SourceCapabilitySnapshot capability;
    SourceEvidenceSnapshot evidence;
    RedactedSourceUri redacted_source_label;
    OpaqueSourceAccessRef access_ref;
    std::optional<std::string> safe_metadata_hint;
};

struct DirectStreamInputResult {
    std::optional<DirectStreamInput> input;
    std::optional<SourceError> error;
    bool is_success() const { return input.has_value(); }
};

}  // namespace kivo::video::source_core
