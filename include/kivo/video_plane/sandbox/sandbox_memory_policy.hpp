#pragma once

#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxMemoryPolicy {
    uint64_t max_shared_memory_bytes{256 * 1024 * 1024};    // 256 MiB
    uint64_t max_process_working_set_bytes{512 * 1024 * 1024};
    bool zero_on_allocate{true};
    bool zero_on_free{true};
    bool operator==(SandboxMemoryPolicy const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
