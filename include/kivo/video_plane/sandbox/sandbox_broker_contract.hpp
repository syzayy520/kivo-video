#pragma once

#include "kivo/video_plane/sandbox/sandbox_ipc_protocol_version.hpp"
#include "kivo/video_plane/sandbox/sandbox_broker_message_kind.hpp"
#include "kivo/video_plane/sandbox/demux_buffer_placement.hpp"
#include <vector>

namespace kivo::video_plane::sandbox {

struct P5SandboxBrokerContract {
    SandboxIpcProtocolVersion ipc_version;
    std::vector<SandboxBrokerMessageKind> supported_messages;
    DemuxBufferPlacement buffer_placement{DemuxBufferPlacement::CopyBridge};
    size_t max_message_payload_size{16 * 1024 * 1024};  // 16 MiB

    // Contract rules:
    // - Broker crash → DemuxBackendCrashedEvidence
    // - Broker timeout → DeadlineExceeded, quarantined
    // - Local trusted file MAY run in-process (with allocation/fuzz constraints)
    // - Cross-process copy count enters evidence
    // - Shared memory: owner + zeroing policy required
    bool operator==(P5SandboxBrokerContract const&) const = default;
};

}  // namespace kivo::video_plane::sandbox
