#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
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

/// SourceSession exposes only session id, state, and contract version.
/// No handles, file descriptors, provider objects, or native objects.
struct SourceSession {
    SourceSessionId id;
    SourceSessionState state{SourceSessionState::created};
    std::uint32_t contract_version{kSourceCoreContractVersion};

    std::string safe_debug() const {
        return "SourceSession{id=" + std::to_string(id.value) +
               ", state=" + std::to_string(static_cast<int>(state)) +
               ", version=" + std::to_string(contract_version) + "}";
    }
};

}  // namespace kivo::video::source_core
