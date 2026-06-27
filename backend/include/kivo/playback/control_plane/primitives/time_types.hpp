#pragma once

#include <cstdint>
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::primitives {

struct MonotonicTimestamp
{
    U64 ticks_ns{0};
    constexpr MonotonicTimestamp() noexcept = default;
    constexpr explicit MonotonicTimestamp(U64 ns) noexcept : ticks_ns(ns) {}
};

struct Deadline
{
    MonotonicTimestamp at{};
    bool infinite{true};
    constexpr Deadline() noexcept = default;
    constexpr explicit Deadline(MonotonicTimestamp t) noexcept : at(t), infinite(false) {}
    static constexpr Deadline make_infinite() noexcept { return Deadline{}; }
};

struct ReadDeadline
{
    Deadline value{};
    constexpr ReadDeadline() noexcept = default;
    constexpr explicit ReadDeadline(Deadline d) noexcept : value(d) {}
};

struct PullDeadline
{
    Deadline value{};
    constexpr PullDeadline() noexcept = default;
    constexpr explicit PullDeadline(Deadline d) noexcept : value(d) {}
};

struct MediaTimeMs
{
    I64 value{0};
    constexpr MediaTimeMs() noexcept = default;
    constexpr explicit MediaTimeMs(I64 v) noexcept : value(v) {}
    constexpr bool is_valid_vod_absolute() const noexcept { return value >= 0; }
};

struct MediaDeltaMs
{
    I64 value{0};
    constexpr MediaDeltaMs() noexcept = default;
    constexpr explicit MediaDeltaMs(I64 v) noexcept : value(v) {}
};

} // namespace kivo::playback::control_plane::primitives
