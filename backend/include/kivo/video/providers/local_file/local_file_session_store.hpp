#pragma once

#include "kivo/video/source_core/runtime/session/source_session.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace kivo::video::providers::local_file {

struct LocalFileSessionRecord {
    source_core::SourceSession session;
    source_core::SourceIdentity identity;
    source_core::SourceCapabilitySnapshot capability;
    source_core::SourceEvidenceSnapshot evidence;
    std::uint64_t current_offset{0};
    std::string internal_safe_path;       // provider-private, never leaked
    std::uint64_t file_size{0};

    bool is_open() const {
        return session.session_state == source_core::SourceSessionState::open;
    }
};

class LocalFileSessionStore {
public:
    source_core::SourceSessionId allocate_id();

    void insert(LocalFileSessionRecord record);
    LocalFileSessionRecord* find(source_core::SourceSessionId id);
    const LocalFileSessionRecord* find(source_core::SourceSessionId id) const;

    void update_offset(source_core::SourceSessionId id, std::uint64_t offset);
    void append_evidence(source_core::SourceSessionId id, source_core::SourceEvidenceItem item);
    source_core::SourceError close_session(source_core::SourceSessionId id);

    // Thread-safety basic boundary
    std::mutex& mutex() { return mutex_; }

private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, LocalFileSessionRecord> records_;
    std::unordered_map<std::uint64_t, LocalFileSessionRecord> tombstones_;
    std::mutex mutex_;
};

}  // namespace kivo::video::providers::local_file
