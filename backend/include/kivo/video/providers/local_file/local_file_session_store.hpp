#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/error/source_error.hpp"
#include "kivo/video/source_core/runtime/session/opaque_source_access_ref.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <functional>
#include <vector>

namespace kivo::video::providers::local_file {

struct LocalFileSessionRecord {
    source_core::SourceSession session;
    source_core::SourceIdentity identity;
    source_core::SourceCapabilitySnapshot capability;
    source_core::SourceEvidenceSnapshot evidence;
    std::uint64_t current_offset{0};
    std::string internal_safe_path;
    std::uint64_t file_size{0};

    bool is_open() const {
        return session.session_state == source_core::SourceSessionState::open;
    }
};

/// Thread-safe session store using copy-snapshot + dedicated mutation methods.
/// No mutable raw pointer returned outside locks.
class LocalFileSessionStore {
public:
    source_core::SourceSessionId allocate_id();

    void insert(LocalFileSessionRecord record);

    // Copy-snapshot for reads (not mutable reference/pointer)
    std::optional<LocalFileSessionRecord> snapshot(source_core::SourceSessionId id) const;

    // Dedicated mutation methods
    void update_offset(source_core::SourceSessionId id, std::uint64_t offset);
    void append_evidence(source_core::SourceSessionId id, source_core::SourceEvidenceItem item);
    source_core::SourceError close_session(source_core::SourceSessionId id);

    // DirectInput snapshots
    source_core::OpaqueSourceAccessRef make_access_ref(source_core::SourceSessionId id) const;

    // Thread-safety: callbacks under lock
    template<typename F>
    void with_record(source_core::SourceSessionId id, F&& callback) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(id.value);
        if (it != records_.end()) callback(it->second);
    }

private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, LocalFileSessionRecord> records_;
    std::unordered_map<std::uint64_t, LocalFileSessionRecord> tombstones_;
    mutable std::mutex mutex_;
};

}  // namespace kivo::video::providers::local_file
