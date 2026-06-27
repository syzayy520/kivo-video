// opaque_attachment_flow_handle.hpp — GATE-021: P5A opaque attachment flow handle
#pragma once
#include <cstdint>

namespace kivo::video_plane::attachment {

struct OpaqueAttachmentFlowHandle {
    uint64_t value{0};                      // opaque handle to attachment flow
    bool is_valid() const { return value != 0; }
    bool operator==(OpaqueAttachmentFlowHandle const&) const = default;
};

}  // namespace kivo::video_plane::attachment
