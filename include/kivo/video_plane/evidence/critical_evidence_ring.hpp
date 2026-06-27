#pragma once

#include <cstdint>
#include <cstddef>

namespace kivo::video_plane::evidence {

struct P5CriticalEvidenceRing {
    // Ring buffer for Fatal/Security/PolicyDenied/FailClosed evidence
    static constexpr size_t kDefaultCapacity = 128;
    static constexpr size_t kMinReservedForTerminalOutcome = 8;

    // Contract:
    // - Must not overwrite current terminal outcome record
    // - Must not silently drop Fatal-severity evidence
    // - Overflow → emit EvidenceRingOverflowEvidence
    // - Read access: P5DiagnosticsService, P8 capability display
    size_t capacity{kDefaultCapacity};
    size_t current_size{0};
    bool terminal_outcome_present{false};
    bool operator==(P5CriticalEvidenceRing const&) const = default;
};

}  // namespace kivo::video_plane::evidence
