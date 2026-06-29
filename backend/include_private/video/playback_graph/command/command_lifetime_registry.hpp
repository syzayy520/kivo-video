#pragma once

#include <cstddef>
#include <vector>

#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "video/playback_graph/command/command_conflict_matrix.hpp"
#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"

namespace kivo::video::playback_graph::runtime {

enum class RegistryThreadRole {
    GraphOwnerThread,
    PublicQueryThread,
    HotPathThread
};

struct CommandCreateRequest {
    GraphCommandKind kind{GraphCommandKind::Open};
    GraphCorrelationId correlation_id{};
    GraphSnapshotRevision accepted_graph_revision{};
    bool requires_sidecar{true};
};

struct CommandCreateResult {
    CommandToken token{};
    PlaybackGraphError error{PlaybackGraphError::None};

    [[nodiscard]] bool accepted() const noexcept {
        return error == PlaybackGraphError::None && token.accepted();
    }
};

class CommandLifetimeRegistry {
public:
    explicit CommandLifetimeRegistry(std::size_t capacity, std::size_t close_reserved_slots);

    void set_sidecar_install_available(bool available) noexcept;
    void set_queue_available(bool available) noexcept;

    [[nodiscard]] CommandCreateResult create(const CommandCreateRequest& request,
                                             P4StateSyncAdapter& p4) noexcept;
    [[nodiscard]] bool mark_running(PlaybackCommandId id) noexcept;
    [[nodiscard]] bool complete(PlaybackCommandId id,
                                CommandTerminalStatus status,
                                PlaybackGraphError error) noexcept;
    [[nodiscard]] bool expire_to_tombstone(PlaybackCommandId id) noexcept;
    [[nodiscard]] bool remove_sidecar(PlaybackCommandId id, RegistryThreadRole role) noexcept;

    [[nodiscard]] CommandLifecycleSnapshot query(PlaybackCommandId id) const noexcept;
    [[nodiscard]] std::size_t active_record_count() const noexcept;
    [[nodiscard]] std::size_t tombstone_count() const noexcept;
    [[nodiscard]] std::uint32_t late_event_discard_count() const noexcept;
    [[nodiscard]] bool has_sidecar(PlaybackCommandId id) const noexcept;

private:
    struct CommandRecord {
        PlaybackCommandId command_id{};
        GraphCorrelationId correlation_id{};
        GraphCommandKind kind{GraphCommandKind::Open};
        CommandLifecycleState state{CommandLifecycleState::Accepted};
        CommandTerminalStatus terminal_status{CommandTerminalStatus::None};
        PlaybackGraphError error{PlaybackGraphError::None};
        GraphSnapshotRevision accepted_graph_revision{};
        GraphSnapshotRevision last_update_graph_revision{};
        bool sidecar_installed{false};
    };

    [[nodiscard]] CommandRecord* find_active(PlaybackCommandId id) noexcept;
    [[nodiscard]] const CommandRecord* find_active(PlaybackCommandId id) const noexcept;
    [[nodiscard]] const CommandLifecycleSnapshot* find_tombstone(PlaybackCommandId id) const noexcept;
    [[nodiscard]] bool has_capacity_for(GraphCommandKind kind) const noexcept;
    [[nodiscard]] CommandCreateResult reject(PlaybackGraphError error) const noexcept;

    std::size_t capacity_{0};
    std::size_t close_reserved_slots_{0};
    bool sidecar_install_available_{true};
    bool queue_available_{true};
    std::vector<CommandRecord> active_records_;
    std::vector<CommandLifecycleSnapshot> tombstones_;
    std::uint32_t late_event_discard_count_{0};
};

}  // namespace kivo::video::playback_graph::runtime
