#pragma once

#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxResourceBudget {
    uint32_t max_processes{4};
    uint64_t max_shared_memory_bytes{256 * 1024 * 1024};  // 256 MiB
    uint64_t max_total_memory_bytes{512 * 1024 * 1024};   // 512 MiB
    bool operator==(SandboxResourceBudget const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
