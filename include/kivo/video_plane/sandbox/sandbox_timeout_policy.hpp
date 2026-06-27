#pragma once

#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxTimeoutPolicy {
    uint64_t open_timeout_ms{10000};
    uint64_t probe_timeout_ms{5000};
    uint64_t find_stream_info_timeout_ms{10000};
    uint64_t read_packet_timeout_ms{3000};
    uint64_t seek_timeout_ms{5000};
    uint64_t flush_timeout_ms{3000};
    uint64_t terminate_timeout_ms{5000};

    // Timeout → DeadlineExceeded evidence + quarantined if repeated
    bool operator==(SandboxTimeoutPolicy const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
