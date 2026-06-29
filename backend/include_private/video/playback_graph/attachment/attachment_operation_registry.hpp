#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/playback_graph/generation_stamp.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"

namespace kivo::video::playback_graph::runtime {

enum class AttachmentKind {
    Source,
    Demux,
    Audio,
    Video
};

enum class AttachmentOperationKind {
    Open,
    Start,
    Flush,
    Seek,
    AudioSeek,
    VideoSeek,
    Drain,
    Close,
    Recovery
};

enum class AttachmentCompletionStatus {
    Succeeded,
    Failed,
    Cancelled,
    TimedOut
};

enum class AttachmentTimeoutReason {
    OperationTimeout,
    PhaseTimeout,
    CommandTimeout
};

struct OperationKey {
    PlaybackSessionId session_id{};
    PlaybackGraphId graph_id{};
    AttachmentInstanceId attachment_instance_id{};
    AttachmentKind attachment_kind{AttachmentKind::Source};
    AttachmentOperationId operation_id{};
    AttachmentOperationKind operation_kind{AttachmentOperationKind::Open};

    bool operator==(const OperationKey&) const = default;
};

struct AttachmentAsyncToken {
    OperationKey key{};
    GenerationStamp generation{};
    PlaybackCommandId command_id{};
    GraphCorrelationId correlation_id{};
};

struct AttachmentCompletionEvent {
    AttachmentAsyncToken token{};
    AttachmentCompletionStatus status{AttachmentCompletionStatus::Succeeded};
    PlaybackGraphError error{PlaybackGraphError::None};
};

struct AttachmentOperationTombstone {
    OperationKey key{};
    AttachmentTimeoutReason reason{AttachmentTimeoutReason::OperationTimeout};
    PlaybackGraphError error{PlaybackGraphError::AttachmentCompletionTimeout};
};

class AttachmentOperationRegistry {
public:
    AttachmentOperationRegistry(std::size_t active_capacity,
                                std::size_t completion_capacity,
                                std::size_t reserved_close_or_recovery_capacity);

    [[nodiscard]] bool start_operation(const AttachmentAsyncToken& token,
                                       bool close_or_recovery) noexcept;
    [[nodiscard]] bool enqueue_completion(const AttachmentCompletionEvent& event) noexcept;
    [[nodiscard]] std::optional<AttachmentCompletionEvent> pop_completion() noexcept;
    [[nodiscard]] bool mark_timeout(const OperationKey& key,
                                    AttachmentTimeoutReason reason,
                                    PlaybackGraphError error) noexcept;

    [[nodiscard]] bool contains_active(const OperationKey& key) const noexcept;
    [[nodiscard]] bool has_tombstone(const OperationKey& key) const noexcept;
    [[nodiscard]] std::size_t active_count() const noexcept;
    [[nodiscard]] std::size_t completion_count() const noexcept;
    [[nodiscard]] std::size_t tombstone_count() const noexcept;
    [[nodiscard]] std::uint32_t late_completion_discard_count() const noexcept;
    [[nodiscard]] std::uint32_t queue_full_drop_count() const noexcept;

private:
    [[nodiscard]] bool has_active_capacity(bool close_or_recovery) const noexcept;

    std::size_t active_capacity_{0};
    std::size_t completion_capacity_{0};
    std::size_t reserved_close_or_recovery_capacity_{0};
    std::vector<AttachmentAsyncToken> active_;
    std::vector<AttachmentCompletionEvent> completions_;
    std::vector<AttachmentOperationTombstone> tombstones_;
    std::uint32_t late_completion_discard_count_{0};
    std::uint32_t queue_full_drop_count_{0};
};

}  // namespace kivo::video::playback_graph::runtime
