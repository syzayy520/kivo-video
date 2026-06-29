#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

struct PlaybackSessionId {
    std::uint64_t value{0};
    bool operator==(const PlaybackSessionId&) const = default;
};

struct PlaybackGraphId {
    std::uint64_t value{0};
    bool operator==(const PlaybackGraphId&) const = default;
};

struct PlaybackCommandId {
    std::uint64_t value{0};
    bool operator==(const PlaybackCommandId&) const = default;
};

struct GraphCorrelationId {
    std::uint64_t value{0};
    bool operator==(const GraphCorrelationId&) const = default;
};

struct PlaybackRevision {
    std::uint64_t value{0};
    bool operator==(const PlaybackRevision&) const = default;
};

struct GraphSnapshotRevision {
    std::uint64_t value{0};
    bool operator==(const GraphSnapshotRevision&) const = default;
};

struct GenerationTupleId {
    std::uint64_t value{0};
    bool operator==(const GenerationTupleId&) const = default;
};

struct AttachmentInstanceId {
    std::uint64_t value{0};
    bool operator==(const AttachmentInstanceId&) const = default;
};

struct AttachmentOperationId {
    std::uint64_t value{0};
    bool operator==(const AttachmentOperationId&) const = default;
};

struct SubscriptionToken {
    std::uint64_t value{0};
    bool operator==(const SubscriptionToken&) const = default;
};

}  // namespace kivo::video::playback_graph
