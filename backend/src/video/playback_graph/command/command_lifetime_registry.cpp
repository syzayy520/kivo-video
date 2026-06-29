#include "video/playback_graph/command/command_lifetime_registry.hpp"

#include <algorithm>

namespace kivo::video::playback_graph::runtime {

CommandLifetimeRegistry::CommandLifetimeRegistry(std::size_t capacity,
                                                 std::size_t close_reserved_slots)
    : capacity_(capacity),
      close_reserved_slots_(close_reserved_slots > capacity ? capacity : close_reserved_slots) {}

void CommandLifetimeRegistry::set_sidecar_install_available(bool available) noexcept {
    sidecar_install_available_ = available;
}

void CommandLifetimeRegistry::set_queue_available(bool available) noexcept {
    queue_available_ = available;
}

CommandCreateResult CommandLifetimeRegistry::create(const CommandCreateRequest& request,
                                                    P4StateSyncAdapter& p4) noexcept {
    if (!has_capacity_for(request.kind)) {
        return reject(PlaybackGraphError::CommandRegistryFull);
    }

    const auto p4_command_id = p4.allocate_command_id();
    if (!p4_command_id.has_value()) {
        return reject(PlaybackGraphError::P4CommandCreateFailed);
    }

    if (request.requires_sidecar && !sidecar_install_available_) {
        return reject(PlaybackGraphError::SidecarInstallFailed);
    }

    if (!queue_available_) {
        return reject(PlaybackGraphError::CommandQueueOverflow);
    }

    CommandRecord record{};
    record.command_id = *p4_command_id;
    record.correlation_id = request.correlation_id;
    record.kind = request.kind;
    record.state = CommandLifecycleState::Enqueued;
    record.accepted_graph_revision = request.accepted_graph_revision;
    record.last_update_graph_revision = request.accepted_graph_revision;
    record.sidecar_installed = request.requires_sidecar;
    active_records_.push_back(record);

    CommandToken token{};
    token.command_id = record.command_id;
    token.correlation_id = record.correlation_id;
    token.accepted_snapshot_revision = request.accepted_graph_revision;
    return CommandCreateResult{token, PlaybackGraphError::None};
}

bool CommandLifetimeRegistry::mark_running(PlaybackCommandId id) noexcept {
    auto* record = find_active(id);
    if (record == nullptr) {
        return false;
    }
    record->state = CommandLifecycleState::Running;
    record->last_update_graph_revision.value += 1;
    return true;
}

bool CommandLifetimeRegistry::complete(PlaybackCommandId id,
                                       CommandTerminalStatus status,
                                       PlaybackGraphError error) noexcept {
    auto* record = find_active(id);
    if (record == nullptr) {
        if (find_tombstone(id) != nullptr) {
            late_event_discard_count_ += 1;
        }
        return false;
    }

    record->terminal_status = status;
    record->error = error;
    record->last_update_graph_revision.value += 1;
    switch (status) {
        case CommandTerminalStatus::Completed:
        case CommandTerminalStatus::NoOp:
            record->state = CommandLifecycleState::Completed;
            break;
        case CommandTerminalStatus::Cancelled:
            record->state = CommandLifecycleState::Cancelled;
            break;
        case CommandTerminalStatus::Superseded:
            record->state = CommandLifecycleState::Superseded;
            break;
        case CommandTerminalStatus::Expired:
            record->state = CommandLifecycleState::Expired;
            break;
        case CommandTerminalStatus::Failed:
        case CommandTerminalStatus::Rejected:
            record->state = CommandLifecycleState::Failed;
            break;
        case CommandTerminalStatus::None:
            break;
    }
    return true;
}

bool CommandLifetimeRegistry::expire_to_tombstone(PlaybackCommandId id) noexcept {
    auto it = std::find_if(active_records_.begin(), active_records_.end(), [id](const auto& record) {
        return record.command_id == id;
    });
    if (it == active_records_.end()) {
        return false;
    }

    CommandLifecycleSnapshot tombstone{};
    tombstone.command_id = it->command_id;
    tombstone.correlation_id = it->correlation_id;
    tombstone.state = CommandLifecycleState::Tombstoned;
    tombstone.terminal_status = CommandTerminalStatus::Expired;
    tombstone.error = PlaybackGraphError::None;
    tombstone.accepted_snapshot_revision = it->accepted_graph_revision;
    tombstone.last_update_snapshot_revision = it->last_update_graph_revision;
    tombstones_.push_back(tombstone);
    active_records_.erase(it);
    return true;
}

bool CommandLifetimeRegistry::remove_sidecar(PlaybackCommandId id, RegistryThreadRole role) noexcept {
    if (role != RegistryThreadRole::GraphOwnerThread) {
        return false;
    }

    auto* record = find_active(id);
    if (record == nullptr) {
        return false;
    }

    record->sidecar_installed = false;
    return true;
}

CommandLifecycleSnapshot CommandLifetimeRegistry::query(PlaybackCommandId id) const noexcept {
    if (const auto* record = find_active(id); record != nullptr) {
        CommandLifecycleSnapshot snapshot{};
        snapshot.command_id = record->command_id;
        snapshot.correlation_id = record->correlation_id;
        snapshot.state = record->state;
        snapshot.terminal_status = record->terminal_status;
        snapshot.error = record->error;
        snapshot.accepted_snapshot_revision = record->accepted_graph_revision;
        snapshot.last_update_snapshot_revision = record->last_update_graph_revision;
        return snapshot;
    }
    if (const auto* tombstone = find_tombstone(id); tombstone != nullptr) {
        return *tombstone;
    }
    return CommandLifecycleSnapshot{};
}

std::size_t CommandLifetimeRegistry::active_record_count() const noexcept {
    return active_records_.size();
}

std::size_t CommandLifetimeRegistry::tombstone_count() const noexcept {
    return tombstones_.size();
}

std::uint32_t CommandLifetimeRegistry::late_event_discard_count() const noexcept {
    return late_event_discard_count_;
}

bool CommandLifetimeRegistry::has_sidecar(PlaybackCommandId id) const noexcept {
    const auto* record = find_active(id);
    return record != nullptr && record->sidecar_installed;
}

CommandLifetimeRegistry::CommandRecord* CommandLifetimeRegistry::find_active(
    PlaybackCommandId id) noexcept {
    auto it = std::find_if(active_records_.begin(), active_records_.end(), [id](const auto& record) {
        return record.command_id == id;
    });
    return it == active_records_.end() ? nullptr : &(*it);
}

const CommandLifetimeRegistry::CommandRecord* CommandLifetimeRegistry::find_active(
    PlaybackCommandId id) const noexcept {
    auto it = std::find_if(active_records_.begin(), active_records_.end(), [id](const auto& record) {
        return record.command_id == id;
    });
    return it == active_records_.end() ? nullptr : &(*it);
}

const CommandLifecycleSnapshot* CommandLifetimeRegistry::find_tombstone(
    PlaybackCommandId id) const noexcept {
    auto it = std::find_if(tombstones_.begin(), tombstones_.end(), [id](const auto& tombstone) {
        return tombstone.command_id == id;
    });
    return it == tombstones_.end() ? nullptr : &(*it);
}

bool CommandLifetimeRegistry::has_capacity_for(GraphCommandKind kind) const noexcept {
    if (active_records_.size() >= capacity_) {
        return false;
    }
    if (kind == GraphCommandKind::Close) {
        return true;
    }
    const auto normal_capacity = capacity_ - close_reserved_slots_;
    return active_records_.size() < normal_capacity;
}

CommandCreateResult CommandLifetimeRegistry::reject(PlaybackGraphError error) const noexcept {
    CommandToken token{};
    token.immediate_error = error;
    return CommandCreateResult{token, error};
}

}  // namespace kivo::video::playback_graph::runtime
