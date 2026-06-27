#pragma once

#include <cstdint>
#include <optional>
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::primitives {

/// Source offset in bytes.
struct SourceOffset
{
    Bytes value{0};
    constexpr SourceOffset() noexcept = default;
    constexpr explicit SourceOffset(Bytes v) noexcept : value(v) {}
};

/// Source range [begin, end_exclusive).
/// begin <= end_exclusive. begin == end_exclusive means empty range.
/// Unknown range uses std::optional<SourceRange>, NOT sentinel max.
struct SourceRange
{
    SourceOffset begin{};
    SourceOffset end_exclusive{};

    constexpr SourceRange() noexcept = default;
    constexpr SourceRange(SourceOffset b, SourceOffset e) noexcept : begin(b), end_exclusive(e) {}

    constexpr bool is_empty() const noexcept
    {
        return begin.value.value >= end_exclusive.value.value;
    }

    constexpr bool is_valid() const noexcept
    {
        return begin.value.value <= end_exclusive.value.value;
    }

    constexpr Bytes size() const noexcept
    {
        if (!is_valid()) return Bytes{0};
        return Bytes{end_exclusive.value.value - begin.value.value};
    }
};

} // namespace kivo::playback::control_plane::primitives
