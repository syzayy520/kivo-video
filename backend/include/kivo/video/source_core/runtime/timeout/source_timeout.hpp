#pragma once

#include <cstdint>
#include <limits>

namespace kivo::video::source_core {

struct SourceTimeout {
    std::uint64_t millis{0};

    static constexpr std::uint64_t infinite() {
        return std::numeric_limits<std::uint64_t>::max();
    }

    bool is_infinite() const { return millis == infinite(); }
};

}  // namespace kivo::video::source_core
