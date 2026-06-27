#pragma once

#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxCrashPolicy {
    uint32_t max_crash_count_before_quarantine{3};
    uint64_t crash_window_seconds{60};
    uint64_t restart_delay_ms{1000};
    bool restart_on_crash{true};
    bool emit_evidence_on_crash{true};

    // Contract:
    // - Crash → DemuxBackendCrashedEvidence entry
    // - After max_crash_count → quarantined, no restart
    // - Shared memory after crash: quarantine + zero
    bool operator==(SandboxCrashPolicy const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
