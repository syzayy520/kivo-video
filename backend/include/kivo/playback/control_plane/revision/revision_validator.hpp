#pragma once

#include "kivo/playback/control_plane/revision/revision_stamp.hpp"

namespace kivo::playback::control_plane::revision {

/// RevisionValidator provides lock-free validation for ReaderWorker.
/// Must NOT take SessionState lock. Must NOT wait for Control Thread.
/// Must NOT trigger callback. Must NOT allocate heap.
class RevisionValidator
{
public:
    bool validate_fast(const RevisionStamp& stamp) const noexcept
    {
        return stamp.session_id.value == current_.session_id.value
            && stamp.generation.value == current_.generation.value;
    }

    RevisionStamp current_stamp_snapshot() const noexcept { return current_; }

    void update(const RevisionStamp& new_stamp) noexcept { current_ = new_stamp; }

private:
    RevisionStamp current_{};
};

} // namespace kivo::playback::control_plane::revision
