#pragma once

#include <atomic>
#include "kivo/playback/control_plane/clock/clock_observation_slot.hpp"

namespace kivo::playback::control_plane::runtime_fake {

/// ControlledClock is a test ClockObservationSlot with controllable time.
/// Realtime-safe publish uses sequence counter + double-buffer (no mutex, no heap).
class ControlledClock : public clock::ClockObservationSlot
{
public:
    void publish_latest_realtime_safe(const clock::ClockObservation& observation) override
    {
        const uint32_t slot = (write_seq_.load(std::memory_order_relaxed) + 1) & 1;
        slots_[slot] = observation;
        write_seq_.store(slot, std::memory_order_release);
    }

    void publish_latest_non_realtime(const clock::ClockObservation& observation) override
    {
        publish_latest_realtime_safe(observation);
    }

    std::optional<clock::ClockObservation> load_latest(clock::ClockSource source) const override
    {
        const uint32_t slot = write_seq_.load(std::memory_order_acquire);
        auto obs = slots_[slot];
        if (obs.source != source) return std::nullopt;
        if (write_seq_.load(std::memory_order_acquire) != slot) return std::nullopt;
        return obs;
    }

    std::optional<clock::ClockObservation> snapshot() const override
    {
        const uint32_t slot = write_seq_.load(std::memory_order_acquire);
        return slots_[slot];
    }

private:
    std::atomic<uint32_t> write_seq_{0};
    clock::ClockObservation slots_[2]{};
};

} // namespace kivo::playback::control_plane::runtime_fake
