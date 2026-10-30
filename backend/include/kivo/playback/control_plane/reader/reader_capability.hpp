#pragma once

#include <optional>
#include "kivo/playback/control_plane/reader/reader_transport_kind.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::reader {

/// ReaderCapability describes reader capabilities.
/// Provider name must NOT enter this struct (only ReaderTransportKind).
struct ReaderCapability
{
    bool seekable{false};
    bool supports_fast_seek{false};
    bool supports_graceful_cancel{false};
    bool supports_destructive_cancel{false};
    bool cancel_interrupts_blocking_io{false};
    bool safe_to_reuse_after_cancel{false};
    bool safe_to_reuse_after_seek{false};
    bool supports_range_read{false};
    std::optional<primitives::Milliseconds> graceful_cancel_bound_ms{};
    std::optional<primitives::Milliseconds> destructive_cancel_bound_ms{};
    ReaderTransportKind transport_kind{ReaderTransportKind::UnknownRemote};
};

} // namespace kivo::playback::control_plane::reader
