#pragma once

#include <array>
#include <cstdint>

#include "kivo/video/playback_graph/generation_stamp.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph::runtime {

enum class SeekPhase {
    ValidateTarget,
    FreezeTransport,
    SourceSeek,
    DemuxSeek,
    AudioFlushBegin,
    VideoFlushBegin,
    AudioSeek,
    VideoSeek,
    ClockReset,
    PublishGeneration,
    CommitP4Revision,
    PublishGraphSnapshot,
    ReleaseBarrier,
    CompleteLogicalSeek
};

struct SeekPlan {
    std::int64_t target_timeline_ms{0};
    bool selected_audio{true};
    bool selected_video{true};
};

struct SeekOutcome {
    bool accepted{false};
    bool logical_complete{false};
    bool background_recycle_pending{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    PlaybackRevision playback_revision{};
    GraphSnapshotRevision graph_snapshot_revision{};
    GenerationStamp generation{};
    bool clock_reset{false};
    std::array<SeekPhase, 14> phases{};
    std::uint32_t phase_count{0};
};

class SeekCoordinator {
public:
    [[nodiscard]] SeekOutcome run_fake_seek(const SeekPlan& plan) noexcept;

private:
    PlaybackRevision next_playback_revision_{1};
    GraphSnapshotRevision next_snapshot_revision_{1};
    GenerationTupleId next_generation_tuple_{1};
};

}  // namespace kivo::video::playback_graph::runtime
