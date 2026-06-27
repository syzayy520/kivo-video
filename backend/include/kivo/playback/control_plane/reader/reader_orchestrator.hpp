#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/handoff/playback_open_envelope.hpp"
#include "kivo/playback/control_plane/revision/cancellation.hpp"
#include "kivo/playback/control_plane/control/seek_target.hpp"
#include "kivo/playback/control_plane/reader/reader_read_result.hpp"

namespace kivo::playback::control_plane::reader {

/// ReadPriority enumerates read scheduling priority.
enum class ReadPriority
{
    Normal,
    Urgent,
    Critical
};

/// ReaderOrchestrator manages reader lifecycle with clear mutation boundaries.
/// Boundaries:
/// - May mutate reader-local state and registry state.
/// - Must NOT mutate SessionState.
/// - Must NOT mutate TimelineState.
/// - Must NOT publish SessionEvent directly.
/// - All session-visible effects go through ControlEventQueue.
class ReaderOrchestrator
{
public:
    virtual ~ReaderOrchestrator() = default;

    virtual void open_initial_reader(const handoff::PlaybackOpenEnvelope& envelope) = 0;
    virtual void schedule_read(ReadPriority priority) = 0;
    virtual void cancel_inflight(revision::CancellationScope scope) = 0;
    virtual void seek_or_replace_reader(const control::SeekTarget& target) = 0;
    virtual void close_reader(bool force) = 0;
    virtual void replace_reader(const handoff::P3ReaderFactoryRef& fallback) = 0;
};

} // namespace kivo::playback::control_plane::reader
