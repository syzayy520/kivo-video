// secure_decode_lifecycle.hpp — P5D D7: SecureDecodeSessionContext lifecycle
#pragma once
#include <cstdint>
#include "../protected_content/secure_decode_session_context_handle.hpp"

namespace kivo::video_plane::decoder {

enum class SecureDecodeState {
    NotCreated,
    Created,
    Active,
    Suspended,
    Destroyed
};

class FakeSecureDecodeLifecycle {
public:
    protected_content::SecureDecodeSessionContextHandle create() {
        handle_.value = ++next_handle_;
        state_ = SecureDecodeState::Created;
        return handle_;
    }

    void activate() {
        if (state_ == SecureDecodeState::Created) state_ = SecureDecodeState::Active;
    }

    void suspend() {
        if (state_ == SecureDecodeState::Active) state_ = SecureDecodeState::Suspended;
    }

    void resume() {
        if (state_ == SecureDecodeState::Suspended) state_ = SecureDecodeState::Active;
    }

    void destroy() {
        state_ = SecureDecodeState::Destroyed;
        handle_.value = 0;
    }

    // BoundaryOnly: handle must not cross P5 boundary as raw pointer
    bool is_boundary_safe() const {
        return handle_.value != 0;  // opaque handle, not pointer
    }

    SecureDecodeState state() const { return state_; }
    protected_content::SecureDecodeSessionContextHandle handle() const { return handle_; }

private:
    protected_content::SecureDecodeSessionContextHandle handle_;
    SecureDecodeState state_{SecureDecodeState::NotCreated};
    uint64_t next_handle_{0};
};

}  // namespace kivo::video_plane::decoder
