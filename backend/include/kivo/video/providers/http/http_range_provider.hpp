#pragma once
#include "kivo/video/source_core/runtime/open/source_open_request.hpp"
#include "kivo/video/source_core/runtime/open/source_open_result.hpp"
#include "kivo/video/source_core/runtime/read/source_read_request.hpp"
#include "kivo/video/source_core/runtime/read/source_read_result.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_request.hpp"
#include "kivo/video/source_core/runtime/seek/source_seek_result.hpp"
#include "kivo/video/source_core/contracts/evidence/source_evidence_snapshot.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_play_input.hpp"
#include "kivo/video/source_core/contracts/decision_input/direct_stream_input.hpp"
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <cstdint>
namespace kivo::video::providers::http {
struct HttpTransportResponse { int status{0}; std::unordered_map<std::string,std::string> headers;
    std::vector<std::uint8_t> body; bool body_cap_hit{false}; };
struct HttpTransportRequest { std::string method; std::string url; std::unordered_map<std::string,std::string> headers;
    std::optional<std::uint64_t> range_start; std::optional<std::uint64_t> range_end; std::uint64_t body_cap{0}; };
using HttpTransportFn = std::function<HttpTransportResponse(const HttpTransportRequest&)>;
class HttpRangeProvider {
public:
    static constexpr std::uint64_t kMaxReadChunkSize = 8*1024*1024;
    static source_core::SourceOpenResult open(const source_core::SourceOpenRequest& req, HttpTransportFn transport,
        struct HttpRangeSessionStore& store);
    static source_core::SourceReadResult read(const source_core::SourceReadRequest& req, HttpTransportFn transport,
        struct HttpRangeSessionStore& store);
    static source_core::SourceSeekResult seek(const source_core::SourceSeekRequest& req, struct HttpRangeSessionStore& store);
    static source_core::SourceError close(source_core::SourceSessionId sid, struct HttpRangeSessionStore& store);
    static source_core::DirectPlayInputResult make_direct_play_input(source_core::SourceSessionId sid, struct HttpRangeSessionStore& store);
    static source_core::DirectStreamInputResult make_direct_stream_input(source_core::SourceSessionId sid, struct HttpRangeSessionStore& store);
    static source_core::SourceEvidenceSnapshot get_evidence(source_core::SourceSessionId sid, struct HttpRangeSessionStore& store);
};

struct HttpRangeSessionRecord {
    source_core::SourceSession session; source_core::SourceIdentity identity;
    source_core::SourceCapabilitySnapshot capability; source_core::SourceEvidenceSnapshot evidence;
    std::uint64_t current_offset{0}; std::uint64_t content_length{0}; std::string redacted_url;
    bool can_read{false};
    bool is_open() const { return session.session_state == source_core::SourceSessionState::open; }
};

struct HttpRangeSessionStore {
    mutable std::mutex m;
    std::uint64_t next_id{1};
    std::unordered_map<std::uint64_t,HttpRangeSessionRecord> records;
    std::unordered_map<std::uint64_t,HttpRangeSessionRecord> tombstones;
    source_core::SourceSessionId allocate_id();
    void insert(HttpRangeSessionRecord r);
    std::optional<HttpRangeSessionRecord> snapshot(source_core::SourceSessionId id) const;
    void update_offset(source_core::SourceSessionId id, std::uint64_t offset);
    void append_evidence(source_core::SourceSessionId id, source_core::SourceEvidenceItem item);
    source_core::SourceError close_session(source_core::SourceSessionId id);
    source_core::OpaqueSourceAccessRef make_access_ref(source_core::SourceSessionId id) const;
};
}  // namespace kivo::video::providers::http
