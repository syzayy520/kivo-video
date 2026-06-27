// swapchain_runtime.hpp -- P5F F3+F4+F5: DXGI swapchain runtime (backbuffer ref, waitable)
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace kivo::video_plane::gpu_render {

enum class SwapchainState {
    NotCreated,
    Created,
    Resizing,
    Resized,
    Released
};

enum class WaitableLifecycleState {
    NotCreated,
    Unsupported,
    CreatedWithFlag,
    WaitHandleAcquired,
    WaitHandleFailed,
    WaitingBeforeRender,
    TimeoutDegraded,
    Closed
};

struct BackBufferRef {
    uint64_t backbuffer_id{0};
    uint64_t generation{0};     // parent backbuffer generation
    uint32_t ref_count{0};      // direct reference count
    bool indirect_ref{false};   // indirect reference (deferred command list)
};

class FakeSwapchainRuntime {
public:
    bool create(uint32_t buffer_count = 2) {
        if (state_ != SwapchainState::NotCreated && state_ != SwapchainState::Released) return false;
        state_ = SwapchainState::Created;
        generation_ = 0;
        backbuffers_.clear();
        for (uint32_t i = 0; i < buffer_count; ++i) {
            backbuffers_.push_back({static_cast<uint64_t>(i) + 1, generation_, 0, false});
        }
        return true;
    }

    // F4: Register direct reference
    bool add_direct_ref(uint64_t backbuffer_id) {
        auto* bb = find_backbuffer(backbuffer_id);
        if (!bb) return false;
        bb->ref_count++;
        return true;
    }

    // F4: Register indirect reference (deferred command list)
    bool add_indirect_ref(uint64_t backbuffer_id) {
        auto* bb = find_backbuffer(backbuffer_id);
        if (!bb) return false;
        bb->indirect_ref = true;
        return true;
    }

    // F4: Release direct reference
    bool release_direct_ref(uint64_t backbuffer_id) {
        auto* bb = find_backbuffer(backbuffer_id);
        if (!bb || bb->ref_count == 0) return false;
        bb->ref_count--;
        return true;
    }

    // F4: Check all refs released before resize
    bool all_refs_released() const {
        for (auto const& bb : backbuffers_) {
            if (bb.ref_count > 0 || bb.indirect_ref) return false;
        }
        return true;
    }

    // F3: Resize (must release all refs first)
    bool resize() {
        if (!all_refs_released()) return false;
        state_ = SwapchainState::Resizing;
        generation_++;
        for (auto& bb : backbuffers_) {
            bb.generation = generation_;
            bb.ref_count = 0;
            bb.indirect_ref = false;
        }
        state_ = SwapchainState::Resized;
        return true;
    }

    // F5: Waitable swapchain lifecycle
    bool enable_waitable(uint32_t max_latency) {
        if (state_ != SwapchainState::Created && state_ != SwapchainState::Resized) return false;
        waitable_state_ = WaitableLifecycleState::CreatedWithFlag;
        max_latency_ = max_latency;
        waitable_state_ = WaitableLifecycleState::WaitHandleAcquired;
        return true;
    }

    void wait_before_render() {
        if (waitable_state_ == WaitableLifecycleState::WaitHandleAcquired) {
            waitable_state_ = WaitableLifecycleState::WaitingBeforeRender;
        }
    }

    void simulate_wait_timeout() {
        waitable_state_ = WaitableLifecycleState::TimeoutDegraded;
    }

    void close() {
        state_ = SwapchainState::Released;
        waitable_state_ = WaitableLifecycleState::Closed;
    }

    SwapchainState state() const { return state_; }
    WaitableLifecycleState waitable_state() const { return waitable_state_; }
    uint64_t generation() const { return generation_; }
    uint32_t max_latency() const { return max_latency_; }
    size_t backbuffer_count() const { return backbuffers_.size(); }

    std::vector<BackBufferRef> const& backbuffers() const { return backbuffers_; }

private:
    BackBufferRef* find_backbuffer(uint64_t id) {
        for (auto& bb : backbuffers_) {
            if (bb.backbuffer_id == id) return &bb;
        }
        return nullptr;
    }

    SwapchainState state_{SwapchainState::NotCreated};
    WaitableLifecycleState waitable_state_{WaitableLifecycleState::NotCreated};
    uint64_t generation_{0};
    uint32_t max_latency_{0};
    std::vector<BackBufferRef> backbuffers_;
};

}  // namespace kivo::video_plane::gpu_render
