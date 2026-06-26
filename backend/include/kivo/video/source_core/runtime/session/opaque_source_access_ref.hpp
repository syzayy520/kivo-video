#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cstdint>

namespace kivo::video::source_core {

/// Opaque source-access reference for DirectInput handoff.
/// No raw path, no native handle, no token or credential.
/// Must be resolved through provider/session store each time.
/// Close makes the ref stale — DirectInput holding this ref does not own file access.
struct OpaqueSourceAccessRef {
    SourceSessionId session_id;
    ProviderKind provider_kind{ProviderKind::unknown};
    std::uint32_t contract_schema_version{kSourceCoreContractVersion};
};

}  // namespace kivo::video::source_core
