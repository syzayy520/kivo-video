#pragma once

#include <cstdint>

namespace kivo::video::source_core {

/// Cancel token for source operations.
/// This skeleton does NOT provide thread-safety guarantees.
struct SourceCancelToken {
    bool cancelled{false};

    void cancel() { cancelled = true; }
    bool is_cancelled() const { return cancelled; }
};

}  // namespace kivo::video::source_core
