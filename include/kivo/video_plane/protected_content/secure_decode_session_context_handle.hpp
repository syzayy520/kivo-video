// secure_decode_session_context_handle.hpp — GATE-011: P5A secure decode session context handle
#pragma once
#include <cstdint>

namespace kivo::video_plane::protected_content {

struct SecureDecodeSessionContextHandle {
    uint64_t value{0};                      // opaque handle to secure decode session
    bool is_valid() const { return value != 0; }
    bool operator==(SecureDecodeSessionContextHandle const&) const = default;
};

}  // namespace kivo::video_plane::protected_content
