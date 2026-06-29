#pragma once

#include <cstddef>
#include <vector>

#include "video/playback_graph/attachment/attachment_operation_registry.hpp"

namespace kivo::video::playback_graph::runtime {

enum class PhaseCompletionResult {
    Accepted,
    DuplicateCompletion,
    UnknownOperation,
    WrongOperationKind,
    WrongGeneration,
    AlreadyTerminal,
    TooManyOperations
};

class PhaseBitset {
public:
    [[nodiscard]] bool add_required(const OperationKey& key,
                                    const GenerationStamp& generation,
                                    AttachmentOperationKind kind) noexcept {
        if (entries_.size() >= kMaxOperationCount || find_entry(key) != nullptr) {
            return false;
        }
        entries_.push_back(Entry{key, generation, kind});
        return true;
    }

    [[nodiscard]] PhaseCompletionResult complete(const OperationKey& key,
                                                 const GenerationStamp& generation,
                                                 AttachmentOperationKind kind,
                                                 AttachmentCompletionStatus status) noexcept {
        auto* entry = find_entry(key);
        if (entry == nullptr) {
            return PhaseCompletionResult::UnknownOperation;
        }
        if (entry->kind != kind) {
            return PhaseCompletionResult::WrongOperationKind;
        }
        if (!(entry->generation == generation)) {
            return PhaseCompletionResult::WrongGeneration;
        }
        if (entry->completed || entry->failed || entry->timed_out) {
            return PhaseCompletionResult::DuplicateCompletion;
        }

        switch (status) {
            case AttachmentCompletionStatus::Succeeded:
                entry->completed = true;
                break;
            case AttachmentCompletionStatus::Failed:
            case AttachmentCompletionStatus::Cancelled:
                entry->failed = true;
                break;
            case AttachmentCompletionStatus::TimedOut:
                entry->timed_out = true;
                break;
        }
        return PhaseCompletionResult::Accepted;
    }

    [[nodiscard]] std::size_t required_count() const noexcept {
        return entries_.size();
    }

    [[nodiscard]] std::size_t completed_count() const noexcept {
        std::size_t count = 0;
        for (const auto& entry : entries_) {
            if (entry.completed) {
                count += 1;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t failed_count() const noexcept {
        std::size_t count = 0;
        for (const auto& entry : entries_) {
            if (entry.failed) {
                count += 1;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t timed_out_count() const noexcept {
        std::size_t count = 0;
        for (const auto& entry : entries_) {
            if (entry.timed_out) {
                count += 1;
            }
        }
        return count;
    }

    static constexpr std::size_t kMaxOperationCount = 64;

private:
    struct Entry {
        OperationKey key{};
        GenerationStamp generation{};
        AttachmentOperationKind kind{AttachmentOperationKind::Open};
        bool completed{false};
        bool failed{false};
        bool timed_out{false};
    };

    [[nodiscard]] Entry* find_entry(const OperationKey& key) noexcept {
        for (auto& entry : entries_) {
            if (entry.key == key) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::vector<Entry> entries_{};
};

}  // namespace kivo::video::playback_graph::runtime
