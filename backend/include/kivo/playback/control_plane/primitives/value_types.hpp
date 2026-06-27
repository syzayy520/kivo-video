#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace kivo::playback::control_plane::primitives {

using U64 = std::uint64_t;
using I64 = std::int64_t;
using U32 = std::uint32_t;

struct Milliseconds
{
    U64 value{0};
    constexpr explicit Milliseconds(U64 v) noexcept : value(v) {}
    constexpr Milliseconds() noexcept = default;
    constexpr Milliseconds checked_add(Milliseconds other) const noexcept { return Milliseconds{value + other.value}; }
    constexpr std::optional<Milliseconds> saturating_add(Milliseconds other) const noexcept
    {
        const U64 max = std::numeric_limits<U64>::max();
        if (value > max - other.value) return std::nullopt;
        return Milliseconds{value + other.value};
    }
    constexpr Milliseconds checked_sub(Milliseconds other) const noexcept
    {
        return Milliseconds{value >= other.value ? value - other.value : 0};
    }
};

struct Microseconds
{
    U64 value{0};
    constexpr explicit Microseconds(U64 v) noexcept : value(v) {}
    constexpr Microseconds() noexcept = default;
};

struct Nanoseconds
{
    U64 value{0};
    constexpr explicit Nanoseconds(U64 v) noexcept : value(v) {}
    constexpr Nanoseconds() noexcept = default;
};

struct Bytes
{
    U64 value{0};
    constexpr explicit Bytes(U64 v) noexcept : value(v) {}
    constexpr Bytes() noexcept = default;
    constexpr Bytes checked_add(Bytes other) const noexcept { return Bytes{value + other.value}; }
    constexpr std::optional<Bytes> saturating_add(Bytes other) const noexcept
    {
        const U64 max = std::numeric_limits<U64>::max();
        if (value > max - other.value) return std::nullopt;
        return Bytes{value + other.value};
    }
    constexpr Bytes checked_sub(Bytes other) const noexcept
    {
        return Bytes{value >= other.value ? value - other.value : 0};
    }
};

struct BitsPerSecond
{
    U64 value{0};
    constexpr explicit BitsPerSecond(U64 v) noexcept : value(v) {}
    constexpr BitsPerSecond() noexcept = default;
};

struct BytesPerSecond
{
    U64 value{0};
    constexpr explicit BytesPerSecond(U64 v) noexcept : value(v) {}
    constexpr BytesPerSecond() noexcept = default;
    static constexpr std::optional<BytesPerSecond> from_bps(BitsPerSecond bps) noexcept
    {
        if (bps.value % 8 != 0) return std::nullopt;
        return BytesPerSecond{bps.value / 8};
    }
};

struct Count
{
    U64 value{0};
    constexpr explicit Count(U64 v) noexcept : value(v) {}
    constexpr Count() noexcept = default;
    constexpr Count checked_add(Count other) const noexcept { return Count{value + other.value}; }
    constexpr std::optional<Count> saturating_add(Count other) const noexcept
    {
        const U64 max = std::numeric_limits<U64>::max();
        if (value > max - other.value) return std::nullopt;
        return Count{value + other.value};
    }
    constexpr Count checked_sub(Count other) const noexcept
    {
        return Count{value >= other.value ? value - other.value : 0};
    }
};

} // namespace kivo::playback::control_plane::primitives
