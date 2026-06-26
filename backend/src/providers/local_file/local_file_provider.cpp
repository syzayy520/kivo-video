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
    if (!val_err.is_ok()) return source_core::SourceOpenResult::failure(val_err);

    std::uint64_t file_size = 0, mtime = 0;
    auto probe_err = probe_file(raw, file_size, mtime);
    if (!probe_err.is_ok()) return source_core::SourceOpenResult::failure(probe_err);

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

    evidence.items.push_back({source_core::SourceEvidencePassKind::contract_pass,
        source_core::SourceEvidenceKind::contract_declared,
        source_core::SourceEvidenceOperation::unknown,
        "contract declared", std::nullopt, source_core::ProviderKind::local_file, 1});
    evidence.items.push_back({source_core::SourceEvidencePassKind::runtime_pass,
        source_core::SourceEvidenceKind::provider_runtime_observed,
        source_core::SourceEvidenceOperation::open,
        "open succeeded", redacted, source_core::ProviderKind::local_file, 2});

    auto sid = store.allocate_id();
    source_core::SourceSession session;
    session.session_id = sid;
    session.session_state = source_core::SourceSessionState::open;
    session.provider_kind = source_core::ProviderKind::local_file;

    LocalFileSessionRecord rec;
    rec.session = session; rec.identity = identity;
    rec.capability = capability; rec.evidence = evidence;
    rec.current_offset = 0; rec.internal_safe_path = raw; rec.file_size = file_size;
    store.insert(std::move(rec));
    return source_core::SourceOpenResult::success(session);
}

// ── Read ──────────────────────────────────────────────
source_core::SourceReadResult LocalFileProvider::read(
    const source_core::SourceReadRequest& request,
    LocalFileSessionStore& store) {

    // Validate session
    auto opt_rec = store.snapshot(request.session_id);
    if (!opt_rec) return {0, false, false, {source_core::SourceErrorCode::session_not_found, ""}, {}};
    auto rec = *opt_rec;
    if (!rec.is_open()) return {0, false, false, {source_core::SourceErrorCode::session_closed, ""}, {}};

    // Zero-size read: no I/O, no offset change, no evidence
    if (request.length == 0) {
        bool eof = rec.current_offset >= rec.file_size;
        return {0, eof, false, source_core::SourceError::ok(), {}};
    }

    // Max chunk guard
    if (request.length > kMaxReadChunkSize)
        return {0, false, false, {source_core::SourceErrorCode::read_size_exceeded, ""}, {}};

    // Determine read offset
    bool is_positioned = request.offset.has_value();
    std::uint64_t read_offset = is_positioned ? request.offset.value() : rec.current_offset;

    // Over-read EOF
    if (read_offset >= rec.file_size)
        return {0, true, false, source_core::SourceError::ok(), {}};

    // Source mutation best-effort
    std::uint64_t curr_sz = 0, curr_mt = 0;
    probe_file(rec.internal_safe_path, curr_sz, curr_mt);

    std::uint64_t safe_len = std::min(request.length, rec.file_size - read_offset);

    // Actual file read
    std::ifstream file(rec.internal_safe_path, std::ios::binary);
    if (!file) return {0, false, false, {source_core::SourceErrorCode::internal_error, "cannot open file for read"}, {}};
    file.seekg(static_cast<std::streamoff>(read_offset));
    if (!file) return {0, false, false, {source_core::SourceErrorCode::seek_unsupported, ""}, {}};

    std::vector<std::uint8_t> buf(safe_len);
    file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(safe_len));
    std::streamsize actual = file.gcount();

    bool eof = (read_offset + static_cast<std::uint64_t>(actual)) >= rec.file_size;

    // Update current_offset for sequential read
    if (!is_positioned && actual > 0)
        store.update_offset(request.session_id, read_offset + static_cast<std::uint64_t>(actual));

    // Append runtime evidence
    if (actual > 0) {
        auto op = is_positioned ? source_core::SourceEvidenceOperation::positioned_read
                                : source_core::SourceEvidenceOperation::read;
        store.append_evidence(request.session_id,
            {source_core::SourceEvidencePassKind::runtime_pass,
             source_core::SourceEvidenceKind::provider_runtime_observed,
             op, "read ok", std::nullopt, source_core::ProviderKind::local_file, 0});
    }

    buf.resize(static_cast<std::size_t>(actual));
    return {static_cast<std::uint64_t>(actual), eof, false, source_core::SourceError::ok(), std::move(buf)};
}

// ── Seek ──────────────────────────────────────────────
source_core::SourceSeekResult LocalFileProvider::seek(
    const source_core::SourceSeekRequest& request,
    LocalFileSessionStore& store) {

    auto opt_rec = store.snapshot(request.session_id);
    if (!opt_rec) return {std::nullopt, {source_core::SourceErrorCode::session_not_found, ""}};
    auto rec = *opt_rec;
    if (!rec.is_open()) return {std::nullopt, {source_core::SourceErrorCode::session_closed, ""}};

    std::int64_t target;
    switch (request.origin) {
    case source_core::SeekOrigin::begin:  target = request.offset; break;
    case source_core::SeekOrigin::current: target = static_cast<std::int64_t>(rec.current_offset) + request.offset; break;
    case source_core::SeekOrigin::end:    target = static_cast<std::int64_t>(rec.file_size) + request.offset; break;
    default: return {std::nullopt, {source_core::SourceErrorCode::seek_unsupported, ""}};
    }
    if (target < 0) return {std::nullopt, {source_core::SourceErrorCode::seek_out_of_range, ""}};
    auto ut = static_cast<std::uint64_t>(target);
    if (ut > rec.file_size) return {std::nullopt, {source_core::SourceErrorCode::seek_out_of_range, ""}};

    store.update_offset(request.session_id, ut);
    store.append_evidence(request.session_id,
        {source_core::SourceEvidencePassKind::runtime_pass,
         source_core::SourceEvidenceKind::range_proof,
         source_core::SourceEvidenceOperation::seek, "seek ok", std::nullopt,
         source_core::ProviderKind::local_file, 0});

    return {ut, source_core::SourceError::ok()};
}

// ── Close ─────────────────────────────────────────────
source_core::SourceError LocalFileProvider::close(
    source_core::SourceSessionId session_id,
    LocalFileSessionStore& store) {
    // Append evidence BEFORE close moves the session to tombstones
    auto opt = store.snapshot(session_id);
    if (opt && opt->session.session_state == source_core::SourceSessionState::open) {
        store.append_evidence(session_id,
            {source_core::SourceEvidencePassKind::runtime_pass,
             source_core::SourceEvidenceKind::session_closed,
             source_core::SourceEvidenceOperation::close, "close ok", std::nullopt,
             source_core::ProviderKind::local_file, 0});
    }
    return store.close_session(session_id);
}

// ── DirectInput ───────────────────────────────────────
source_core::DirectPlayInputResult LocalFileProvider::make_direct_play_input(
    source_core::SourceSessionId session_id,
    LocalFileSessionStore& store) {
    auto opt_rec = store.snapshot(session_id);
    if (!opt_rec) return {std::nullopt, source_core::SourceError{source_core::SourceErrorCode::session_not_found, ""}};
    auto rec = *opt_rec;
    if (rec.session.session_state == source_core::SourceSessionState::closed)
        return {std::nullopt, source_core::SourceError{source_core::SourceErrorCode::session_closed, ""}};

    source_core::DirectPlayInput dpi;
    dpi.identity = rec.identity;
    dpi.capability = rec.capability;
    dpi.evidence = rec.evidence;
    dpi.redacted_source_label = rec.identity.display_uri;
    dpi.access_ref = store.make_access_ref(session_id);
    return {std::move(dpi), std::nullopt};
}

source_core::DirectStreamInputResult LocalFileProvider::make_direct_stream_input(
    source_core::SourceSessionId session_id,
    LocalFileSessionStore& store) {
    auto opt_rec = store.snapshot(session_id);
    if (!opt_rec) return {std::nullopt, source_core::SourceError{source_core::SourceErrorCode::session_not_found, ""}};
    auto rec = *opt_rec;
    if (rec.session.session_state == source_core::SourceSessionState::closed)
        return {std::nullopt, source_core::SourceError{source_core::SourceErrorCode::session_closed, ""}};

    source_core::DirectStreamInput dsi;
    dsi.identity = rec.identity;
    dsi.capability = rec.capability;
    dsi.evidence = rec.evidence;
    dsi.redacted_source_label = rec.identity.display_uri;
    dsi.access_ref = store.make_access_ref(session_id);
    return {std::move(dsi), std::nullopt};
}

// ── Evidence ──────────────────────────────────────────
source_core::SourceEvidenceSnapshot LocalFileProvider::get_evidence(
    source_core::SourceSessionId session_id,
    LocalFileSessionStore& store) {
    auto opt = store.snapshot(session_id);
    if (!opt) return {};
    return opt->evidence;
}

}  // namespace kivo::video::providers::local_file
