// sync_byte_view_guard.hpp — P6B data: RAII guard for AudioSyncByteView
// HEADER-ONLY. Scopes a borrowed byte view so it cannot outlive its guard.
#pragma once
#include <cstdint>
#include <span>
#include "kivo/video/audio_plane/data/audio_sync_byte_view.hpp"

namespace kivo::video::audio_plane::runtime::data {

using kivo::video::audio_plane::data::AudioSyncByteView;

// SyncByteViewGuard
// RAII wrapper around an AudioSyncByteView. The view is valid only while the
// guard is alive. On destruction the guard invalidates the view by resetting
// its byte span to empty, enforcing the sync-call-only / must-not-be-stored
// contract rules from P6A.
class SyncByteViewGuard {
public:
    // Construct from a borrowed view. The guard does not copy bytes; it only
    // holds a reference to the view and invalidates it on destruction.
    explicit SyncByteViewGuard(AudioSyncByteView& view) noexcept
        : view_(view), valid_(true) {}

    SyncByteViewGuard(const SyncByteViewGuard&) = delete;
    SyncByteViewGuard& operator=(const SyncByteViewGuard&) = delete;

    SyncByteViewGuard(SyncByteViewGuard&& other) noexcept
        : view_(other.view_), valid_(other.valid_) {
        other.valid_ = false;
    }

    SyncByteViewGuard& operator=(SyncByteViewGuard&& other) noexcept {
        if (this != &other) {
            invalidate();
            view_ = other.view_;
            valid_ = other.valid_;
            other.valid_ = false;
        }
        return *this;
    }

    // Acquire the view. Only valid while the guard is alive.
    AudioSyncByteView acquire() const noexcept {
        return view_;
    }

    // Debug-mode validity check.
    bool is_valid() const noexcept { return valid_; }

    // Destructor: invalidate the borrowed view (empty span), then release guard.
    ~SyncByteViewGuard() {
        invalidate();
    }

private:
    void invalidate() noexcept {
        if (valid_) {
            // Reset the borrowed view's bytes to an empty span.
            view_.bytes = std::span<const std::byte>{};
            valid_ = false;
        }
    }

    AudioSyncByteView& view_;
    bool valid_;
};

}  // namespace kivo::video::audio_plane::runtime::data
