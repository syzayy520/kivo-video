// fake_spsc_render_queue.hpp — P6B runtime: fake SPSC render queue
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
// FAKE ADAPTER: NOT P6E runtime. Deterministic, single-threaded test use.
// is_lock_free() returns FALSE (honest): real lock-free deferred to P6E.
#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <type_traits>
#include "kivo/video/audio_plane/render/audio_spsc_result.hpp"

namespace kivo::video::audio_plane::runtime::render {

using kivo::video::audio_plane::render::AudioSpscPushResult;
using kivo::video::audio_plane::render::AudioSpscPopResult;
using kivo::video::audio_plane::render::AudioSpscDepthSnapshot;

// FakeSpscRenderQueue
// Template SPSC queue with fixed Capacity. Uses seq_cst atomics for simplicity
// (NOT optimized memory order). is_lock_free() returns false honestly — this
// is a fake adapter; production lock-free is deferred to P6E.
//
// Storage: std::array<Packet, Capacity> with head/tail indices.
template <typename Packet, size_t Capacity>
class FakeSpscRenderQueue {
    static_assert(Capacity > 0, "Capacity must be > 0");
    static_assert(std::is_nothrow_move_constructible_v<Packet>,
                  "Packet must be nothrow move constructible");

public:
    FakeSpscRenderQueue() = default;
    ~FakeSpscRenderQueue() = default;

    FakeSpscRenderQueue(const FakeSpscRenderQueue&) = delete;
    FakeSpscRenderQueue& operator=(const FakeSpscRenderQueue&) = delete;

    // Push a packet. Returns Pushed/Full/Closed/GenerationMismatch/InvalidPacket.
    AudioSpscPushResult try_push(Packet&& packet) noexcept {
        if (closed_.load(std::memory_order_seq_cst)) {
            return AudioSpscPushResult::Closed;
        }
        // Generation check: packet generation 0 is treated as invalid.
        if (!is_packet_valid(packet)) {
            return AudioSpscPushResult::InvalidPacket;
        }
        if (!is_generation_match(packet)) {
            return AudioSpscPushResult::GenerationMismatch;
        }
        const size_t head = head_.load(std::memory_order_seq_cst);
        const size_t next = (head + 1) % Capacity;
        const size_t tail = tail_.load(std::memory_order_seq_cst);
        if (next == tail) {
            return AudioSpscPushResult::Full;
        }
        buffer_[head] = std::move(packet);
        head_.store(next, std::memory_order_seq_cst);
        return AudioSpscPushResult::Pushed;
    }

    // Pop a packet into out. Returns Popped/Empty/Closed/GenerationMismatch.
    AudioSpscPopResult try_pop(Packet& out) noexcept {
        if (closed_.load(std::memory_order_seq_cst)) {
            const size_t tail = tail_.load(std::memory_order_seq_cst);
            const size_t head = head_.load(std::memory_order_seq_cst);
            if (tail == head) {
                return AudioSpscPopResult::Closed;
            }
            // Drain remaining packets even when closed.
            out = std::move(buffer_[tail]);
            tail_.store((tail + 1) % Capacity, std::memory_order_seq_cst);
            return AudioSpscPopResult::Popped;
        }
        const size_t tail = tail_.load(std::memory_order_seq_cst);
        const size_t head = head_.load(std::memory_order_seq_cst);
        if (tail == head) {
            return AudioSpscPopResult::Empty;
        }
        out = std::move(buffer_[tail]);
        tail_.store((tail + 1) % Capacity, std::memory_order_seq_cst);
        return AudioSpscPopResult::Popped;
    }

    // HONEST: this is NOT production lock-free. Real lock-free is P6E.
    bool is_lock_free() const noexcept { return false; }

    // Approximate depth snapshot (single-threaded fake: exact).
    AudioSpscDepthSnapshot depth_snapshot() const noexcept {
        const size_t head = head_.load(std::memory_order_seq_cst);
        const size_t tail = tail_.load(std::memory_order_seq_cst);
        AudioSpscDepthSnapshot snap;
        snap.capacity = static_cast<uint32_t>(Capacity - 1);
        snap.approximate_depth = static_cast<uint32_t>(
            (head + Capacity - tail) % Capacity);
        snap.is_approximate = false;
        return snap;
    }

    // Close the queue. After close, pushes are rejected; pops drain remaining.
    void close() noexcept {
        closed_.store(true, std::memory_order_seq_cst);
    }

    bool is_closed() const noexcept {
        return closed_.load(std::memory_order_seq_cst);
    }

    // Generation control: packets with non-matching generation are rejected.
    void set_generation(uint64_t gen) noexcept {
        generation_.store(gen, std::memory_order_seq_cst);
    }
    uint64_t generation() const noexcept {
        return generation_.load(std::memory_order_seq_cst);
    }

private:
    // Default packet validity check: override-able per specialization if needed.
    // A packet is invalid if its pool_slot_handle_id is 0.
    static bool is_packet_valid(const Packet& p) noexcept {
        return p.pool_slot_handle_id != 0;
    }

    bool is_generation_match(const Packet& p) const noexcept {
        const uint64_t gen = generation_.load(std::memory_order_seq_cst);
        // Generation 0 means "no generation check configured".
        if (gen == 0) return true;
        return p.pipeline_generation.value == gen;
    }

    std::array<Packet, Capacity> buffer_{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    std::atomic<bool> closed_{false};
    std::atomic<uint64_t> generation_{0};
};

}  // namespace kivo::video::audio_plane::runtime::render
