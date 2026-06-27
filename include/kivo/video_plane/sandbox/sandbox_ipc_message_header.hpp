#pragma once

#include <string>
#include <cstdint>

namespace kivo::video_plane::sandbox {

struct SandboxIpcMessageHeader {
    uint32_t protocol_version{0};
    uint64_t request_id{0};
    std::string pipeline_id;
    uint64_t media_epoch{0};
    uint64_t deadline_ms{0};
    uint64_t payload_size{0};
    std::string shared_memory_handle;  // opaque ref, not raw pointer
    uint32_t capability_flags{0};
    uint32_t security_class{0};
    std::string correlation_id;
    uint32_t message_kind{0};          // SandboxBrokerMessageKind cast
    bool operator==(SandboxIpcMessageHeader const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
