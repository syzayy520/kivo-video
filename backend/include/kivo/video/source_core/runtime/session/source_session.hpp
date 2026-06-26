#pragma once

#include "kivo/video/source_core/contracts/provider_kind/provider_kind.hpp"
#include "kivo/video/source_core/contracts/decision_input/source_core_contract_version.hpp"

#include <cstdint>
#include <string>

namespace kivo::video::source_core {

enum class SourceSessionState : std::uint8_t {
    created = 0,
    open = 1,
    closed = 2,
    error = 3
};

struct SourceSessionId {
    std::uint64_t value{0};
    bool operator==(const SourceSessionId& other) const { return value == other.value; }
};

struct SourceSession {
    SourceSessionId session_id;
    SourceSessionState session_state{SourceSessionState::created};
    std::uint32_t contract_version{kSourceCoreContractVersion};
    ProviderKind provider_kind{ProviderKind::unknown};

    std::string debug_string() const {
        return "SourceSession{session_id=" + std::to_string(session_id.value) +
               ", session_state=" + std::to_string(static_cast<int>(session_state)) +
               ", version=" + std::to_string(contract_version) + "}";
    }
};

}  // namespace kivo::video::source_core
