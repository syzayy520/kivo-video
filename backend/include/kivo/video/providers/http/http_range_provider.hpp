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
#include <cstring>
namespace kivo::video::providers::http {

// RR-023: Provider-private URL wrapper with defense-in-depth secure_clear
struct ProviderInternalRemoteUri {
    std::string raw;
    ProviderInternalRemoteUri() = default;
    explicit ProviderInternalRemoteUri(std::string u) : raw(std::move(u)) {}
    ~ProviderInternalRemoteUri() { secure_clear(); }
    void secure_clear() { if(!raw.empty()){ std::memset(raw.data(),0,raw.size()); raw.clear(); } }
    ProviderInternalRemoteUri(const ProviderInternalRemoteUri& o) : raw(o.raw) {}
    ProviderInternalRemoteUri& operator=(const ProviderInternalRemoteUri& o) { raw=o.raw; return *this; }
    ProviderInternalRemoteUri(ProviderInternalRemoteUri&& o) noexcept : raw(std::move(o.raw)) {}
    ProviderInternalRemoteUri& operator=(ProviderInternalRemoteUri&& o) noexcept { raw=std::move(o.raw); return *this; }
};

// RR-021: Redirect policy DTO
struct HttpRedirectChainDigest {
    std::uint32_t hop_count{0}; bool crossed_origin{false}; bool downgraded_scheme{false};
    bool auth_stripped{false}; std::uint32_t redirect_limit{5};
};
struct HttpRedirectPolicy {
    std::uint32_t max_redirects{5}; bool allow_http_to_https{true};
    bool allow_https_to_http{false}; bool allow_cross_origin_auth{false};
    HttpRedirectChainDigest chain;
};

// RR-018/033: Concurrency and connection reuse policy DTOs
struct HttpConcurrencyPolicy {
    std::uint32_t max_concurrent_requests_per_session{4};
    std::uint32_t max_concurrent_requests_per_origin{8};
};
struct HttpConnectionReusePolicy {
    bool enable_keepalive{true}; std::uint32_t idle_timeout_ms{30000};
    std::uint32_t max_connections_per_origin{6};
};

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
    ProviderInternalRemoteUri raw_url;
    bool can_read{false};
    bool is_open() const { return session.session_state == source_core::SourceSessionState::open; }
    void tombstone_sensitive_clear() { raw_url.secure_clear(); }
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
