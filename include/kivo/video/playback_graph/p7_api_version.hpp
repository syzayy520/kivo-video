#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct P7ApiVersion {
    std::uint32_t major{1};
    std::uint32_t minor{9};
    std::uint32_t patch{1};
};

inline constexpr P7ApiVersion kP7ApiVersion{};

}  // namespace kivo::video::playback_graph
