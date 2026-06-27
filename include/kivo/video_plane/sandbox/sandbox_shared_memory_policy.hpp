#pragma once

#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxSharedMemoryPolicy {
    bool require_owner{true};
    bool zero_on_release{true};
    bool quarantine_on_crash{true};
    bool zero_on_quarantine{true};
    uint64_t max_handle_lifetime_ms{60000};
    bool operator==(SandboxSharedMemoryPolicy const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
