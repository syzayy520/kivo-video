#pragma once

#include <cstdint>
#include <numeric>
#include <optional>

namespace kivo::playback::control_plane::primitives {

/// Ratio with gcd canonicalization.
/// denominator != 0. numerator != 0 (unless future 0x rate).
/// {2,2} must canonicalize to {1,1}.
/// Compare using checked 128-bit cross multiply.
/// No implicit construction from double.
struct Ratio
{
    U32 numerator{1};
    U32 denominator{1};

    constexpr Ratio() noexcept = default;

    static std::optional<Ratio> make(U32 num, U32 den) noexcept
    {
        if (den == 0) return std::nullopt;
        if (num == 0) return std::nullopt;
        const U32 g = std::gcd(num, den);
        return Ratio{num / g, den / g};
    }

    constexpr bool operator==(const Ratio& other) const noexcept
    {
        return numerator == other.numerator && denominator == other.denominator;
    }

private:
    constexpr Ratio(U32 num, U32 den) noexcept : numerator(num), denominator(den) {}
};

} // namespace kivo::playback::control_plane::primitives
