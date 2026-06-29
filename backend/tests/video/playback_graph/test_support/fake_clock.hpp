#pragma once

#include <cstdint>

namespace kivo::video::playback_graph::test_support {

class FakeClock {
public:
    void advance_ms(std::uint64_t delta_ms) noexcept {
        now_ms_ += delta_ms;
    }

    [[nodiscard]] std::uint64_t now_ms() const noexcept {
        return now_ms_;
    }

private:
    std::uint64_t now_ms_{0};
};

}  // namespace kivo::video::playback_graph::test_support
