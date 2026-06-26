#pragma once

#include "kivo/video/source_core/contracts/identity/source_identity.hpp"
#include "kivo/video/source_core/contracts/capability/source_capability_snapshot.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_play_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_stream_input.hpp"
#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/open/source_open_result.hpp"
#include "kivo/video/source_core/runtime/read/source_read_request.hpp"
#include "kivo/video/source_core/runtime/read/source_read_result.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_request.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_result.hpp"
#include "kivo/video/source_core/runtime/session/opaque_source_access_ref.hpp"
#include "kivo/video/source_core/contracts/error/source_error.hpp"

#include "local_file_session_store.hpp"

#include <string>
#include <cstdint>

namespace kivo::video::providers::local_file {

class LocalFileProvider {
public:
    static constexpr std::uint64_t kMaxReadChunkSize = 64 * 1024 * 1024;

    // --- Open --- (accepts store reference so read/seek/close can access the same session)
    static source_core::SourceOpenResult open(
        const source_core::SourceOpenRequest& request,
        LocalFileSessionStore& store);

    // --- Read ---
    static source_core::SourceReadResult read(
        const source_core::SourceReadRequest& request,
        LocalFileSessionStore& store);  // non-const: may update offset

    // --- Seek ---
    static source_core::SourceSeekResult seek(
        const source_core::SourceSeekRequest& request,
        LocalFileSessionStore& store);

    // --- Close ---
    static source_core::SourceError close(
        source_core::SourceSessionId session_id,
        LocalFileSessionStore& store);

    // --- DirectInput ---
    static source_core::DirectPlayInput make_direct_play_input(
        source_core::SourceSessionId session_id,
        const LocalFileSessionStore& store);

    static source_core::DirectStreamInput make_direct_stream_input(
        source_core::SourceSessionId session_id,
        const LocalFileSessionStore& store);

    // --- Evidence ---
    static source_core::SourceEvidenceSnapshot get_evidence(
        source_core::SourceSessionId session_id,
        const LocalFileSessionStore& store);
};

}  // namespace kivo::video::providers::local_file
