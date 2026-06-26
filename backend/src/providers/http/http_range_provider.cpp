#include "kivo/video/providers/http/http_range_provider.hpp"
#include "kivo/video/providers/http/http_url.hpp"
#include "kivo/video/providers/http/content_range_parser.hpp"
#include <algorithm>
namespace kivo::video::providers::http {
namespace sc = kivo::video::source_core;

static std::string build_range_header(std::uint64_t start, std::uint64_t end) {
    return "bytes=" + std::to_string(start) + "-" + std::to_string(end);
}

// ── SessionStore ─────────────────────────────────────
sc::SourceSessionId HttpRangeSessionStore::allocate_id() { std::lock_guard lk(m); return sc::SourceSessionId{next_id++}; }
void HttpRangeSessionStore::insert(HttpRangeSessionRecord r) { auto id=r.session.session_id.value; std::lock_guard lk(m); records[id]=std::move(r); }
std::optional<HttpRangeSessionRecord> HttpRangeSessionStore::snapshot(sc::SourceSessionId id) const { std::lock_guard lk(m); auto it=records.find(id.value); if(it!=records.end()) return it->second; auto tit=tombstones.find(id.value); if(tit!=tombstones.end()) return tit->second; return std::nullopt; }
void HttpRangeSessionStore::update_offset(sc::SourceSessionId id, std::uint64_t off) { std::lock_guard lk(m); auto it=records.find(id.value); if(it!=records.end()) it->second.current_offset=off; }
void HttpRangeSessionStore::append_evidence(sc::SourceSessionId id, sc::SourceEvidenceItem item) { std::lock_guard lk(m); auto it=records.find(id.value); if(it!=records.end()) { item.sequence_number=static_cast<std::uint64_t>(it->second.evidence.items.size()+1); it->second.evidence.items.push_back(std::move(item)); } }
sc::SourceError HttpRangeSessionStore::close_session(sc::SourceSessionId id) { std::lock_guard lk(m); auto tit=tombstones.find(id.value); if(tit!=tombstones.end()) return sc::SourceError::ok(); auto it=records.find(id.value); if(it==records.end()) return {sc::SourceErrorCode::session_not_found,""}; it->second.session.session_state=sc::SourceSessionState::closed; it->second.tombstone_sensitive_clear(); tombstones[id.value]=std::move(it->second); records.erase(it); return sc::SourceError::ok(); }
sc::OpaqueSourceAccessRef HttpRangeSessionStore::make_access_ref(sc::SourceSessionId id) const { std::lock_guard lk(m); sc::OpaqueSourceAccessRef ref; ref.session_id=id; ref.provider_kind=sc::ProviderKind::http_file; return ref; }

// ── Open ─────────────────────────────────────────────
sc::SourceOpenResult HttpRangeProvider::open(const sc::SourceOpenRequest& req, HttpTransportFn transport, HttpRangeSessionStore& store) {
    std::string raw = req.uri.untrusted_input_for_open_boundary();
    HttpUrl url; auto err=parse_url(raw,url);
    if (err!=HttpUrlError::none) return sc::SourceOpenResult::failure({sc::SourceErrorCode::invalid_request,"invalid url"});
    if (!transport) return sc::SourceOpenResult::failure({sc::SourceErrorCode::internal_error,"no transport"});

    // Range probe: GET bytes=0-0
    HttpTransportRequest probe; probe.method="GET"; probe.url=raw;
    probe.range_start=0; probe.range_end=0; probe.body_cap=65536;
    auto resp = transport(probe);
    // Reject compressed/multipart/malformed early
    if (resp.status==401||resp.status==403||resp.status==429||resp.status==503) return sc::SourceOpenResult::failure({sc::SourceErrorCode::internal_error,"auth/rate/unavailable"});
    bool has_range=false; std::uint64_t clen=0; sc::RangeProofKind rpk=sc::RangeProofKind::none;

    if (resp.body_cap_hit || resp.status==499) { return sc::SourceOpenResult::failure({sc::SourceErrorCode::internal_error,"body cap/cancelled"}); }
    if (resp.status==200) { rpk=sc::RangeProofKind::rejected_no_range; return sc::SourceOpenResult::failure({sc::SourceErrorCode::range_not_supported,"no range support"}); }
    if (resp.status!=206&&resp.status!=416) return sc::SourceOpenResult::failure({sc::SourceErrorCode::internal_error,"unexpected status:"+std::to_string(resp.status)});

    if (resp.status==206) {
        ParsedContentRange pcr;
        auto ce=parse_content_range(resp.headers["Content-Range"],pcr);
        if (ce==ContentRangeError::none && pcr.valid) {
            if (pcr.has_total) { has_range=true; clen=pcr.total; rpk=sc::RangeProofKind::seekable_known_length; }
            else { has_range=true; clen=0; rpk=sc::RangeProofKind::range_observed_unknown_length_not_seekable; }
        }
    } else if (resp.status==416) {
        ParsedContentRange pcr;
        parse_content_range(resp.headers["Content-Range"],pcr);
        if (pcr.has_total && pcr.total==0) { has_range=true; clen=0; rpk=sc::RangeProofKind::seekable_empty_source; }
        else if (!pcr.has_total) { has_range=true; clen=0; rpk=sc::RangeProofKind::seekable_empty_source; }
    } else if (resp.status==200) {
        rpk=sc::RangeProofKind::rejected_no_range;
        return sc::SourceOpenResult::failure({sc::SourceErrorCode::invalid_request,"no range support"});
    }

    if (!has_range) return sc::SourceOpenResult::failure({sc::SourceErrorCode::range_not_supported,"no valid range"});

    // Build session
    auto sid=store.allocate_id();
    sc::SourceSession sess; sess.session_id=sid; sess.session_state=sc::SourceSessionState::open; sess.provider_kind=sc::ProviderKind::http_file;

    sc::SourceIdentity identity; identity.kind=sc::ProviderKind::http_file; identity.safe_label=url.safe_redacted();

    sc::SourceCapabilitySnapshot cap; cap.seek=sc::CapabilityState::supported; cap.can_range_request=sc::CapabilityState::supported;
    cap.content_length_bytes=clen; cap.range_proof_kind=rpk; cap.eligibility=(rpk==sc::RangeProofKind::seekable_known_length||rpk==sc::RangeProofKind::seekable_empty_source)?sc::SourceAccessEligibility::eligible_seekable:sc::SourceAccessEligibility::ineligible_no_range;

    sc::SourceEvidenceSnapshot ev; ev.kind=sc::ProviderKind::http_file; ev.capability=cap;
    ev.items.push_back({sc::SourceEvidencePassKind::runtime_pass, sc::SourceEvidenceKind::provider_runtime_observed, sc::SourceEvidenceOperation::open, "http range open ok", std::nullopt, sc::ProviderKind::http_file, 1});

    HttpRangeSessionRecord rec; rec.session=sess; rec.identity=identity; rec.capability=cap; rec.evidence=ev;
    rec.content_length=clen; rec.can_read=(rpk==sc::RangeProofKind::seekable_known_length); rec.redacted_url=url.safe_redacted();
    rec.raw_url = ProviderInternalRemoteUri(raw);
    store.insert(std::move(rec));
    return sc::SourceOpenResult::success(sess);
}

// ── Read ─────────────────────────────────────────────
sc::SourceReadResult HttpRangeProvider::read(const sc::SourceReadRequest& req, HttpTransportFn transport, HttpRangeSessionStore& store) {
    auto opt=store.snapshot(req.session_id);
    if (!opt) return {0,false,false,{sc::SourceErrorCode::session_not_found,""},{}};
    auto rec=*opt;
    if (!rec.is_open()) return {0,false,false,{sc::SourceErrorCode::session_closed,""},{}};
    if (req.length==0) { bool eof=rec.current_offset>=rec.content_length; return {0,eof,false,sc::SourceError::ok(),{}}; }
    if (req.length>kMaxReadChunkSize) return {0,false,false,{sc::SourceErrorCode::read_size_exceeded,""},{}};
    bool is_pos=req.offset.has_value();
    std::uint64_t ro=is_pos?req.offset.value():rec.current_offset;
    if (ro>=rec.content_length) return {0,true,false,sc::SourceError::ok(),{}};
    std::uint64_t safe_len=std::min(req.length,rec.content_length-ro);

    HttpTransportRequest hr; hr.method="GET"; hr.url=rec.redacted_url;
    hr.range_start=ro; hr.range_end=ro+safe_len-1; hr.body_cap=safe_len+4096;
    auto resp=transport(hr);
    if (resp.status!=206) return {0,false,false,{sc::SourceErrorCode::internal_error,"read failed:"+std::to_string(resp.status)},{}};

    bool eof=(ro+static_cast<std::uint64_t>(resp.body.size()))>=rec.content_length;
    if (!is_pos && resp.body.size()>0) store.update_offset(req.session_id,ro+static_cast<std::uint64_t>(resp.body.size()));
    if (resp.body.size()>0) store.append_evidence(req.session_id,{sc::SourceEvidencePassKind::runtime_pass, sc::SourceEvidenceKind::provider_runtime_observed,
        is_pos?sc::SourceEvidenceOperation::positioned_read:sc::SourceEvidenceOperation::read,"http read ok",std::nullopt,sc::ProviderKind::http_file,0});

    sc::SourceReadResult result{static_cast<std::uint64_t>(resp.body.size()),eof,false,sc::SourceError::ok(),std::move(resp.body)};
    return result;
}

// ── Seek ─────────────────────────────────────────────
sc::SourceSeekResult HttpRangeProvider::seek(const sc::SourceSeekRequest& req, HttpRangeSessionStore& store) {
    auto opt=store.snapshot(req.session_id);
    if (!opt) return {std::nullopt,{sc::SourceErrorCode::session_not_found,""}};
    auto rec=*opt;
    if (!rec.is_open()) return {std::nullopt,{sc::SourceErrorCode::session_closed,""}};
    std::int64_t tgt;
    switch(req.origin){ case sc::SeekOrigin::begin:tgt=req.offset;break; case sc::SeekOrigin::current:tgt=static_cast<std::int64_t>(rec.current_offset)+req.offset;break;
    case sc::SeekOrigin::end:tgt=static_cast<std::int64_t>(rec.content_length)+req.offset;break; default:return{std::nullopt,{sc::SourceErrorCode::seek_unsupported,""}}; }
    if (tgt<0||static_cast<std::uint64_t>(tgt)>rec.content_length) return{std::nullopt,{sc::SourceErrorCode::seek_out_of_range,""}};
    store.update_offset(req.session_id,static_cast<std::uint64_t>(tgt));
    store.append_evidence(req.session_id,{sc::SourceEvidencePassKind::runtime_pass,sc::SourceEvidenceKind::range_proof,sc::SourceEvidenceOperation::seek,"seek ok",std::nullopt,sc::ProviderKind::http_file,0});
    return {static_cast<std::uint64_t>(tgt),sc::SourceError::ok()};
}

// ── Close ────────────────────────────────────────────
sc::SourceError HttpRangeProvider::close(sc::SourceSessionId sid, HttpRangeSessionStore& store) {
    auto opt=store.snapshot(sid);
    if (opt && opt->is_open()) store.append_evidence(sid,{sc::SourceEvidencePassKind::runtime_pass,sc::SourceEvidenceKind::session_closed,sc::SourceEvidenceOperation::close,"close ok",std::nullopt,sc::ProviderKind::http_file,0});
    return store.close_session(sid);
}

// ── DirectInput ──────────────────────────────────────
sc::DirectPlayInputResult HttpRangeProvider::make_direct_play_input(sc::SourceSessionId sid, HttpRangeSessionStore& store) {
    auto opt=store.snapshot(sid);
    if (!opt) { sc::DirectPlayInputResult r; r.error={sc::SourceErrorCode::session_not_found,""}; return r; }
    if (opt->session.session_state==sc::SourceSessionState::closed) { sc::DirectPlayInputResult r; r.error={sc::SourceErrorCode::session_closed,""}; return r; }
    sc::DirectPlayInput dpi; dpi.identity=opt->identity; dpi.capability=opt->capability; dpi.evidence=opt->evidence; dpi.access_ref=store.make_access_ref(sid);
    sc::DirectPlayInputResult r; r.input=std::move(dpi); return r;
}
sc::DirectStreamInputResult HttpRangeProvider::make_direct_stream_input(sc::SourceSessionId sid, HttpRangeSessionStore& store) {
    auto opt=store.snapshot(sid);
    if (!opt) { sc::DirectStreamInputResult r; r.error={sc::SourceErrorCode::session_not_found,""}; return r; }
    if (opt->session.session_state==sc::SourceSessionState::closed) { sc::DirectStreamInputResult r; r.error={sc::SourceErrorCode::session_closed,""}; return r; }
    sc::DirectStreamInput dsi; dsi.identity=opt->identity; dsi.capability=opt->capability; dsi.evidence=opt->evidence; dsi.access_ref=store.make_access_ref(sid);
    sc::DirectStreamInputResult r; r.input=std::move(dsi); return r;
}
sc::SourceEvidenceSnapshot HttpRangeProvider::get_evidence(sc::SourceSessionId sid, HttpRangeSessionStore& store) { auto opt=store.snapshot(sid); return opt?opt->evidence:sc::SourceEvidenceSnapshot{}; }
}  // namespace kivo::video::providers::http
