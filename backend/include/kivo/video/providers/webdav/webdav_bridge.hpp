#pragma once
#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/open/source_open_result.hpp"
#include "kivo/video/source_core/runtime/read/source_read_result.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_result.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_play_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_stream_input.hpp"
#include "kivo/video/providers/http/http_range_provider.hpp"
#include <string>
#include <cstdint>

namespace kivo::video::providers::webdav {

enum class WebDavProbeResult { file_like=0, collection=1, resource_type_unknown=2, propfind_failed=3, malformed=4 };

struct WebDavInfo { WebDavProbeResult probe{WebDavProbeResult::resource_type_unknown}; std::string safe_label; };

// PROPFIND hint: probe resource type only (Depth: 0, no body parsing needed for MVP)
WebDavInfo probe_resource_type(const std::string& http_url, http::HttpTransportFn transport);

// Bridge: use HTTP Range after PROPFIND
class WebDavRangeBridge {
public:
    static source_core::SourceOpenResult open(
        const source_core::SourceOpenRequest& req, http::HttpTransportFn transport,
        http::HttpRangeSessionStore& store);
    static source_core::SourceReadResult read(const source_core::SourceReadRequest& req,
        http::HttpTransportFn transport, http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::read(req,transport,store); }
    static source_core::SourceSeekResult seek(const source_core::SourceSeekRequest& req,
        http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::seek(req,store); }
    static source_core::SourceError close(source_core::SourceSessionId sid, http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::close(sid,store); }
    static source_core::DirectPlayInputResult make_direct_play_input(source_core::SourceSessionId sid, http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::make_direct_play_input(sid,store); }
    static source_core::DirectStreamInputResult make_direct_stream_input(source_core::SourceSessionId sid, http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::make_direct_stream_input(sid,store); }
    static source_core::SourceEvidenceSnapshot get_evidence(source_core::SourceSessionId sid, http::HttpRangeSessionStore& store)
    { return http::HttpRangeProvider::get_evidence(sid,store); }
};

}  // namespace kivo::video::providers::webdav
