// audio_device_remainder_slot.hpp — P6A §16: single-slot remainder
#pragma once
#include <cstdint>
#include <atomic>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/render/audio_render_packet.hpp"
namespace kivo::video::audio_plane::render {
struct AudioDeviceRemainderSlot {
    ContractMetadata metadata;
    bool has_value{false};
    AudioRenderPacket packet;
    // Rules: AudioDeviceThread only; no heap alloc; no blocking recovery
    static constexpr bool owned_by_device_thread_only{true};
    static constexpr bool prepare_thread_cannot_access{true};
    static constexpr bool no_heap_allocation{true};
    static constexpr bool no_blocking_recovery{true};
    static constexpr bool overwrite_is_critical_invariant_violation{true};
};
struct AudioDeviceRemainderSlotOverwritePolicy {
    std::atomic<uint64_t> compact_critical_counter{0};
    std::atomic<bool> async_recovery_flag{false};
    // Overwrite → set atomic recovery flag + increment counter + return from event loop
};
}  // namespace kivo::video::audio_plane::render
