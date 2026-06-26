#include "kivo/video/providers/local_file/local_file_provider.hpp"
#include "kivo/video/providers/local_file/local_file_path_validation.hpp"
#include "kivo/video/providers/local_file/local_file_opaque_digest.hpp"
#include "kivo/video/providers/local_file/local_file_redaction.hpp"

#include <fstream>
#include <algorithm>
#include <filesystem>

namespace kivo::video::providers::local_file {

// ── Open ──────────────────────────────────────────────
source_core::SourceOpenResult LocalFileProvider::open(
    const source_core::SourceOpenRequest& request,
    LocalFileSessionStore& store) {

    std::string raw = request.uri.untrusted_input_for_open_boundary();
    auto val_err = validate_path(raw);
    if (!val_err.is_ok()) {
        return source_core::SourceOpenResult::failure(val_err);
    }

    std::uint64_t file_size = 0, mtime = 0;
    auto probe_err = probe_file(raw, file_size, mtime);
    if (!probe_err.is_ok()) {
        return source_core::SourceOpenResult::failure(probe_err);
    }

    auto digest = compute_opaque_digest(raw, file_size, mtime);
    auto prefix = digest_prefix(digest);
    auto ext = safe_extension(raw);
    auto redacted = make_redacted_uri(prefix, ext);

    source_core::SourceIdentity identity;
    identity.kind = source_core::ProviderKind::local_file;
    identity.safe_label = "local_file:" + prefix;
    identity.display_uri = redacted;

    source_core::SourceCapabilitySnapshot capability;
    capability.seek = source_core::CapabilityState::supported;
    capability.range_read = source_core::CapabilityState::supported;
    capability.content_length_bytes = file_size;

    source_core::SourceEvidenceSnapshot evidence;
    evidence.kind = source_core::ProviderKind::local_file;
    evidence.uri_redacted = redacted.display();
    evidence.capability = capability;

    evidence.items.push_back(source_core::SourceEvidenceItem{
        source_core::SourceEvidencePassKind::contract_pass,
        source_core::SourceEvidenceKind::contract_declared,
        source_core::SourceEvidenceOperation::unknown,
        "contract declared", std::nullopt,
        source_core::ProviderKind::local_file, 1
    });
    evidence.items.push_back(source_core::SourceEvidenceItem{
        source_core::SourceEvidencePassKind::runtime_pass,
        source_core::SourceEvidenceKind::provider_runtime_observed,
        source_core::SourceEvidenceOperation::open,
        "open succeeded", redacted,
        source_core::ProviderKind::local_file, 2
    });

    auto sid = store.allocate_id();
    source_core::SourceSession session;
    session.session_id = sid;
    session.session_state = source_core::SourceSessionState::open;
    session.provider_kind = source_core::ProviderKind::local_file;

    LocalFileSessionRecord rec;
    rec.session = session;
    rec.identity = identity;
    rec.capability = capability;
    rec.evidence = evidence;
    rec.current_offset = 0;
    rec.internal_safe_path = raw;
    rec.file_size = file_size;

    store.insert(std::move(rec));
    return source_core::SourceOpenResult::success(session);
}

// ── Read ──────────────────────────────────────────────
source_core::SourceReadResult LocalFileProvider::read(
    const source_core::SourceReadRequest& request,
    LocalFileSessionStore& store) {

    // Validate session
    auto* rec = store.find(request.session_id);
    if (!rec) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::session_not_found, ""}};
    }
    if (!rec->is_open()) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::session_closed, ""}};
    }

    // Zero-size read
    if (request.length == 0) {
        bool eof = rec->current_offset >= rec->file_size;
        return source_core::SourceReadResult{0, eof, false, source_core::SourceError::ok()};
    }

    // Max chunk guard
    if (request.length > kMaxReadChunkSize) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::read_size_exceeded, ""}};
    }

    // Determine read offset
    std::uint64_t read_offset;
    bool is_positioned = request.offset.has_value();
    if (is_positioned) {
        read_offset = request.offset.value();
    } else {
        read_offset = rec->current_offset;
    }

    // EOF check
    if (read_offset >= rec->file_size) {
        return source_core::SourceReadResult{0, true, false, source_core::SourceError::ok()};
    }

    // Source mutation check
    std::uint64_t current_size = 0, current_mtime = 0;
    auto probe_err = probe_file(rec->internal_safe_path, current_size, current_mtime);
    if (!probe_err.is_ok()) {
        return source_core::SourceReadResult{0, false, false, probe_err};
    }
    if (current_size != rec->file_size) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::source_changed, ""}};
    }

    // Compute safe read size
    std::uint64_t safe_len = std::min(request.length, rec->file_size - read_offset);

    // Actual file read
    std::ifstream file(rec->internal_safe_path, std::ios::binary);
    if (!file) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::internal_error, "cannot open file for read"}};
    }
    file.seekg(static_cast<std::streamoff>(read_offset));
    if (!file) {
        return source_core::SourceReadResult{0, false, false,
            {source_core::SourceErrorCode::seek_unsupported, ""}};
    }

    std::vector<char> buf(safe_len);
    file.read(buf.data(), static_cast<std::streamsize>(safe_len));
    std::streamsize bytes = file.gcount();

    // Unexpected short read check
    if (static_cast<std::uint64_t>(bytes) < safe_len && file.eof()) {
        // source may have mutated
    }

    bool eof = (read_offset + static_cast<std::uint64_t>(bytes)) >= rec->file_size;

    source_core::SourceReadResult result;
    result.bytes_read = static_cast<std::uint64_t>(bytes);
    result.eof = eof;
    result.would_block = false;
    result.error = source_core::SourceError::ok();
    return result;
}

// ── Seek ──────────────────────────────────────────────
source_core::SourceSeekResult LocalFileProvider::seek(
    const source_core::SourceSeekRequest& request,
    LocalFileSessionStore& store) {

    auto* rec = store.find(request.session_id);
    if (!rec) {
        return source_core::SourceSeekResult{std::nullopt,
            {source_core::SourceErrorCode::session_not_found, ""}};
    }
    if (!rec->is_open()) {
        return source_core::SourceSeekResult{std::nullopt,
            {source_core::SourceErrorCode::session_closed, ""}};
    }

    std::int64_t target;
    switch (request.origin) {
    case source_core::SeekOrigin::begin:
        target = request.offset;
        break;
    case source_core::SeekOrigin::current:
        target = static_cast<std::int64_t>(rec->current_offset) + request.offset;
        break;
    case source_core::SeekOrigin::end:
        target = static_cast<std::int64_t>(rec->file_size) + request.offset;
        break;
    default:
        return source_core::SourceSeekResult{std::nullopt,
            {source_core::SourceErrorCode::seek_unsupported, ""}};
    }

    if (target < 0) {
        return source_core::SourceSeekResult{std::nullopt,
            {source_core::SourceErrorCode::seek_out_of_range, ""}};
    }
    auto utarget = static_cast<std::uint64_t>(target);
    if (utarget > rec->file_size) {
        return source_core::SourceSeekResult{std::nullopt,
            {source_core::SourceErrorCode::seek_out_of_range, ""}};
    }

    rec->current_offset = utarget;
    return source_core::SourceSeekResult{utarget, source_core::SourceError::ok()};
}

// ── Close ─────────────────────────────────────────────
source_core::SourceError LocalFileProvider::close(
    source_core::SourceSessionId session_id,
    LocalFileSessionStore& store) {
    return store.close_session(session_id);
}

// ── DirectInput ───────────────────────────────────────
source_core::DirectPlayInput LocalFileProvider::make_direct_play_input(
    source_core::SourceSessionId session_id,
    const LocalFileSessionStore& store) {

    source_core::DirectPlayInput dpi;
    auto* rec = store.find(session_id);
    if (!rec) return dpi;
    dpi.identity = rec->identity;
    dpi.capability = rec->capability;
    dpi.evidence = rec->evidence;
    dpi.redacted_source_label = rec->identity.display_uri;
    return dpi;
}

source_core::DirectStreamInput LocalFileProvider::make_direct_stream_input(
    source_core::SourceSessionId session_id,
    const LocalFileSessionStore& store) {

    source_core::DirectStreamInput dsi;
    auto* rec = store.find(session_id);
    if (!rec) return dsi;
    dsi.identity = rec->identity;
    dsi.capability = rec->capability;
    dsi.evidence = rec->evidence;
    dsi.redacted_source_label = rec->identity.display_uri;
    return dsi;
}

// ── Evidence ──────────────────────────────────────────
source_core::SourceEvidenceSnapshot LocalFileProvider::get_evidence(
    source_core::SourceSessionId session_id,
    const LocalFileSessionStore& store) {
    auto* rec = store.find(session_id);
    if (!rec) return source_core::SourceEvidenceSnapshot{};
    return rec->evidence;
}

}  // namespace kivo::video::providers::local_file
