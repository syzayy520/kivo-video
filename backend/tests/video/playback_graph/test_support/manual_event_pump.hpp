#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::playback_graph::test_support {

class ManualEventPump {
public:
    [[nodiscard]] bool push(std::uint32_t event_id) noexcept {
        if (count_ >= events_.size()) {
            return false;
        }
        events_[count_++] = event_id;
        return true;
    }

    [[nodiscard]] std::uint32_t run_all() noexcept {
        const auto processed = count_;
        count_ = 0;
        return processed;
    }

private:
    std::array<std::uint32_t, 16> events_{};
    std::uint32_t count_{0};
};

}  // namespace kivo::video::playback_graph::test_support
