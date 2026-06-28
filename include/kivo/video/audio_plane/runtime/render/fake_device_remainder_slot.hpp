// fake_device_remainder_slot.hpp — P6B runtime: fake device remainder slot
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
// FAKE ADAPTER: NOT P6D runtime. No real device thread, no real WASAPI.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/render/audio_render_packet.hpp"

namespace kivo::video::audio_plane::runtime::render {

using kivo::video::audio_plane::render::AudioRenderPacket;

// OverwriteViolation: set() called when the slot already has a value.
// Per P6A §16, overwrite is a critical invariant violation.
enum class OverwriteViolation : int32_t {
    SlotNotEmpty = 0,
};

// SlotEmpty: consume() called when the slot has no value.
enum class SlotEmpty : int32_t {
    NoValue = 0,
};

// FakeDeviceRemainderSlot
// Single-slot remainder store. Deterministic, single-threaded fake.
// - set() fails if the slot already has a value (overwrite = violation).
// - consume() moves the value out and clears the slot.
// - overwrite_count() tracks compact critical counter for violations.
// This is a FAKE ADAPTER, NOT P6D: no real AudioDeviceThread, no real WASAPI.
class FakeDeviceRemainderSlot {
public:
    FakeDeviceRemainderSlot() = default;

    // Set the packet. Fails with OverwriteViolation if slot already has value.
    kivo::Expected<std::monostate, OverwriteViolation> set(AudioRenderPacket packet) {
        if (has_value_) {
            ++overwrite_count_;
            return OverwriteViolation::SlotNotEmpty;
        }
        packet_ = std::move(packet);
        has_value_ = true;
        return std::monostate{};
    }

    // Consume the packet. Fails with SlotEmpty if no value present.
    kivo::Expected<AudioRenderPacket, SlotEmpty> consume() {
        if (!has_value_) {
            return SlotEmpty::NoValue;
        }
        AudioRenderPacket out = std::move(packet_);
        packet_ = AudioRenderPacket{};
        has_value_ = false;
        return out;
    }

    bool has_value() const noexcept { return has_value_; }

    void clear() noexcept {
        packet_ = AudioRenderPacket{};
        has_value_ = false;
    }

    // Compact critical counter: total overwrite violations observed.
    uint64_t overwrite_count() const noexcept { return overwrite_count_; }

private:
    AudioRenderPacket packet_{};
    bool has_value_{false};
    uint64_t overwrite_count_{0};
};

}  // namespace kivo::video::audio_plane::runtime::render
