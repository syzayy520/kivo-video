#pragma once

#include <optional>
#include "kivo/playback/control_plane/primitives/ratio.hpp"

namespace kivo::playback::control_plane::rate {

/// PlaybackRateRatio wraps Ratio.
/// 1.0x = {1,1}, 1.25x = {5,4}, 0.5x = {1,2}.
/// {2,2} must canonicalize to {1,1}.
/// Forbidden: double == 1.0 to check normal rate.
/// Forbidden: implicit construction from double.
struct PlaybackRateRatio
{
    primitives::Ratio value{};

    constexpr PlaybackRateRatio() noexcept = default;

    static std::optional<PlaybackRateRatio> make(primitives::U32 num, primitives::U32 den) noexcept
    {
        auto r = primitives::Ratio::make(num, den);
        if (!r) return std::nullopt;
        return PlaybackRateRatio{*r};
    }

    bool is_normal_rate() const noexcept
    {
        return value.numerator == 1 && value.denominator == 1;
    }

private:
    constexpr explicit PlaybackRateRatio(primitives::Ratio r) noexcept : value(r) {}
};

} // namespace kivo::playback::control_plane::rate
