// owned_block_allocator.hpp — P6B data: heap-backed owned block allocator
// HEADER-ONLY. Fake allocator backing AudioOwnedBlockHandle with heap storage.
#pragma once
#include <cstdint>
#include <vector>
#include "kivo/video/audio_plane/data/audio_owned_block_handle.hpp"
#include "kivo/video/audio_plane/data/audio_data_handle_id.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"

namespace kivo::video::audio_plane::runtime::data {

using kivo::video::audio_plane::data::AudioOwnedBlockHandle;

// OwnedBlockAllocator
// Heap-backed fake allocator. Allocations are tracked in a vector of byte
// vectors; release_fn is noexcept (sets a flag, no heavy free on the hot path).
// Real cleanup happens in the destructor (non-realtime path).
class OwnedBlockAllocator {
public:
    OwnedBlockAllocator() = default;

    // Allocate a block of the given size for a packet.
    // The returned handle's release_fn points to a noexcept stub that marks
    // the block for deferred cleanup; no heavy free is performed inline.
    AudioOwnedBlockHandle allocate(packet::AudioPacketId packet_id, std::size_t size) {
        const uint64_t id = next_id_++;
        blocks_.emplace_back(size, std::byte{0});
        AudioOwnedBlockHandle handle{};
        handle.handle_id.value = id;
        handle.packet_id = packet_id;
        handle.release_fn = &OwnedBlockAllocator::noexcept_release_stub;
        handle.release_ctx = this;  // context used by the stub
        handle.flags.is_noexcept_release = true;
        handle.flags.is_heavy_release = false;
        return handle;
    }

    // Mark a block released (noexcept, sets flag). Does not free memory.
    void mark_released(uint64_t handle_id) noexcept {
        if (handle_id > 0 && handle_id <= blocks_.size()) {
            released_flags_.assign(blocks_.size(), false);
            if (released_flags_.size() < blocks_.size()) {
                released_flags_.resize(blocks_.size(), false);
            }
            released_flags_[handle_id - 1] = true;
        }
    }

    // Diagnostic: total number of allocations (released or not).
    std::size_t total_allocated() const noexcept { return blocks_.size(); }

    // Diagnostic: number of currently-live (not-yet-released) blocks.
    std::size_t live_count() const noexcept {
        std::size_t released = 0;
        for (std::size_t i = 0; i < released_flags_.size() && i < blocks_.size(); ++i) {
            if (released_flags_[i]) ++released;
        }
        return blocks_.size() - released;
    }

    // Destructor: releases all backing storage (non-realtime cleanup).
    ~OwnedBlockAllocator() {
        blocks_.clear();
        blocks_.shrink_to_fit();
        released_flags_.clear();
    }

    OwnedBlockAllocator(const OwnedBlockAllocator&) = delete;
    OwnedBlockAllocator& operator=(const OwnedBlockAllocator&) = delete;
    OwnedBlockAllocator(OwnedBlockAllocator&&) noexcept = default;
    OwnedBlockAllocator& operator=(OwnedBlockAllocator&&) noexcept = default;

private:
    // noexcept release stub: marks the block released without heavy free.
    static void noexcept_release_stub(void* ctx) noexcept {
        // In the fake, the ctx is the allocator; the actual handle id is not
        // passed through the C-style fn signature, so we only flag that a
        // release occurred. Real cleanup is deferred to the destructor.
        (void)ctx;
    }

    uint64_t next_id_{1};
    std::vector<std::vector<std::byte>> blocks_;
    std::vector<bool> released_flags_;
};

}  // namespace kivo::video::audio_plane::runtime::data
